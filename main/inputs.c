/**
 * inputs.c — ODO button + dual rotary encoder, center cluster.
 *
 * GPIO assignments come from Kconfig.projbuild (TC_ENC1_*, TC_ENC2_*, TC_BUTTON_ODO_GPIO).
 *
 * Debounce strategy:
 *   Each GPIO fires an ISR on any edge. The ISR disables itself and arms a
 *   one-shot esp_timer for TC_ENC_DEBOUNCE_MS. The timer callback re-reads
 *   the pin, applies logic, re-enables the ISR. This is lighter than polling
 *   and avoids the LVGL tick dependency in the ISR.
 *
 * LVGL event flow:
 *   Encoder rotation  → updates g_dash.menu_cursor atomically
 *                        → sets g_dash.menu_id (opens popup on right display
 *                          via next UART frame)
 *                        → posts LV_EVENT_KEY to center UI if needed
 *   Encoder press     → confirms selection, closes popup (menu_id = 0),
 *                        queues a CAN TX frame via canbus_tx_selection()
 *   ODO short press   → cycles g_dash.odo_mode
 *   ODO long press    → resets active trip (trip_a or trip_b only)
 *
 * CAN TX:
 *   Center runs TWAI_MODE_NORMAL — receives ECU broadcast frames AND transmits
 *   encoder selection frames (boost map, TC slip) on explicit button press only.
 *   See canbus.h: canbus_tx_selection(uint32_t id, uint8_t index).
 */

#include "inputs.h"
#include "canbus.h"
#include "dash_data.h"
#include "menu_strings.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

static const char *TAG = "inputs";

extern portMUX_TYPE g_dash_mux;

/* ── Menu option tables ───────────────────────────────────────────────
 * Defined in menu_strings.h (shared with right-side-cluster ui_menu_popup.c).
 * Keeping them in a single header prevents silent drift between repos.
 * BOOST_MAPS / TC_SLIPS / BOOST_MAP_COUNT / TC_SLIP_COUNT are all available.
 */
#if 0  /* kept as reference comment only — real strings live in menu_strings.h */
static const char * const TC_SLIPS_REFERENCE[] __attribute__((unused)) = {
    "OFF      0 deg",
    "LOW      3 deg",
    "MED      6 deg",
    "HIGH    10 deg",
    "TRACK   15 deg",
};
#endif  /* reference only */

/* ── State ───────────────────────────────────────────────────────────── */
static volatile int  s_enc1_last_a = 1;   /* Boost encoder last A state */
static volatile int  s_enc2_last_a = 1;   /* TC encoder last A state    */

/* Menu auto-close timer */
static esp_timer_handle_t s_menu_close_timer = NULL;

static void menu_arm_timeout(void);
static void menu_close_cb(void *arg)
{
    portENTER_CRITICAL(&g_dash_mux);
    g_dash.menu_id     = DASH_MENU_NONE;
    g_dash.menu_cursor = 0;
    portEXIT_CRITICAL(&g_dash_mux);
    ESP_LOGD(TAG, "menu closed by timeout");
}

static void menu_arm_timeout(void)
{
#if CONFIG_TC_MENU_TIMEOUT_MS > 0
    esp_timer_stop(s_menu_close_timer);
    esp_timer_start_once(s_menu_close_timer,
                         (uint64_t)CONFIG_TC_MENU_TIMEOUT_MS * 1000ULL);
#endif
}

/* ── Debounce helpers ────────────────────────────────────────────────── */
typedef struct {
    gpio_num_t  gpio;
    esp_timer_handle_t timer;
    void (*on_stable)(int level);
} debounced_gpio_t;

static void IRAM_ATTR debounce_isr(void *arg)
{
    debounced_gpio_t *d = (debounced_gpio_t *)arg;
    gpio_intr_disable(d->gpio);
    esp_timer_start_once(d->timer, (uint64_t)CONFIG_TC_ENC_DEBOUNCE_MS * 1000ULL);
}

static debounced_gpio_t s_enc1_a, s_enc1_b, s_enc1_sw;
static debounced_gpio_t s_enc2_a, s_enc2_b, s_enc2_sw;
static debounced_gpio_t s_odo_btn;

/* ── ODO button ──────────────────────────────────────────────────────── */
#define ODO_LONG_PRESS_MS  1000

static esp_timer_handle_t s_odo_long_timer = NULL;
static volatile bool      s_odo_pressed    = false;
static volatile bool      s_odo_long_fired = false;

static void odo_long_cb(void *arg)
{
    s_odo_long_fired = true;
    /* Long press: reset active trip (never ODO) */
    portENTER_CRITICAL(&g_dash_mux);
    if (g_dash.odo_mode == DASH_TRIP_A) g_dash.trip_a = 0.0f;
    if (g_dash.odo_mode == DASH_TRIP_B) g_dash.trip_b = 0.0f;
    portEXIT_CRITICAL(&g_dash_mux);
    ESP_LOGI(TAG, "trip reset");
}

static void odo_stable_cb(int level)
{
    gpio_intr_enable(s_odo_btn.gpio);
    if (level == 0) {
        /* Button pressed (active-low) */
        s_odo_pressed    = true;
        s_odo_long_fired = false;
        esp_timer_start_once(s_odo_long_timer,
                             (uint64_t)ODO_LONG_PRESS_MS * 1000ULL);
    } else {
        /* Released */
        esp_timer_stop(s_odo_long_timer);
        if (s_odo_pressed && !s_odo_long_fired) {
            /* Short press: cycle mode */
            portENTER_CRITICAL(&g_dash_mux);
            g_dash.odo_mode = (g_dash.odo_mode + 1) % 3;
            portEXIT_CRITICAL(&g_dash_mux);
            ESP_LOGD(TAG, "odo mode → %d", g_dash.odo_mode);
        }
        s_odo_pressed = false;
    }
}

static void odo_timer_cb(void *arg) { odo_stable_cb(gpio_get_level(s_odo_btn.gpio)); }

/* ── Encoder helpers ─────────────────────────────────────────────────── */
static void encoder_step(uint8_t menu_id, int *last_a, int cur_a, int cur_b,
                         int item_count)
{
    if (cur_a == *last_a) return;   /* no change */
    *last_a = cur_a;

    int dir = (cur_a != cur_b) ? 1 : -1;

    portENTER_CRITICAL(&g_dash_mux);
    if (g_dash.menu_id != menu_id) {
        g_dash.menu_id     = menu_id;
        g_dash.menu_cursor = 0;
    } else {
        int next = (int)g_dash.menu_cursor + dir;
        if (next < 0) next = 0;
        if (next >= item_count) next = item_count - 1;
        g_dash.menu_cursor = (uint8_t)next;
    }
    portEXIT_CRITICAL(&g_dash_mux);

    menu_arm_timeout();
    ESP_LOGD(TAG, "menu %u cursor %u", menu_id, g_dash.menu_cursor);
}

/* Encoder 1 (Boost) */
static void enc1_a_stable_cb(int level)
{
    gpio_intr_enable(s_enc1_a.gpio);
    encoder_step(DASH_MENU_BOOST, (int *)&s_enc1_last_a,
                 level, gpio_get_level(s_enc1_b.gpio), BOOST_MAP_COUNT);
}
static void enc1_a_timer_cb(void *a) { enc1_a_stable_cb(gpio_get_level(s_enc1_a.gpio)); }
static void enc1_b_stable_cb(int l)  { gpio_intr_enable(s_enc1_b.gpio); }
static void enc1_b_timer_cb(void *a) { enc1_b_stable_cb(gpio_get_level(s_enc1_b.gpio)); }

static void enc1_sw_stable_cb(int level)
{
    gpio_intr_enable(s_enc1_sw.gpio);
    if (level != 0) return;   /* only on press, active-low */
    uint8_t idx;
    portENTER_CRITICAL(&g_dash_mux);
    idx            = g_dash.menu_cursor;
    g_dash.menu_id = DASH_MENU_NONE;
    portEXIT_CRITICAL(&g_dash_mux);
    esp_timer_stop(s_menu_close_timer);
    canbus_tx_selection(CONFIG_TC_CAN_TX_BOOST_ID, idx);
    ESP_LOGI(TAG, "boost map %u selected → CAN 0x%03X", idx, CONFIG_TC_CAN_TX_BOOST_ID);
}
static void enc1_sw_timer_cb(void *a) { enc1_sw_stable_cb(gpio_get_level(s_enc1_sw.gpio)); }

/* Encoder 2 (TC) */
static void enc2_a_stable_cb(int level)
{
    gpio_intr_enable(s_enc2_a.gpio);
    encoder_step(DASH_MENU_TC, (int *)&s_enc2_last_a,
                 level, gpio_get_level(s_enc2_b.gpio), TC_SLIP_COUNT);
}
static void enc2_a_timer_cb(void *a) { enc2_a_stable_cb(gpio_get_level(s_enc2_a.gpio)); }
static void enc2_b_stable_cb(int l)  { gpio_intr_enable(s_enc2_b.gpio); }
static void enc2_b_timer_cb(void *a) { enc2_b_stable_cb(gpio_get_level(s_enc2_b.gpio)); }

static void enc2_sw_stable_cb(int level)
{
    gpio_intr_enable(s_enc2_sw.gpio);
    if (level != 0) return;
    uint8_t idx;
    portENTER_CRITICAL(&g_dash_mux);
    idx            = g_dash.menu_cursor;
    g_dash.menu_id = DASH_MENU_NONE;
    portEXIT_CRITICAL(&g_dash_mux);
    esp_timer_stop(s_menu_close_timer);
    canbus_tx_selection(CONFIG_TC_CAN_TX_TC_ID, idx);
    ESP_LOGI(TAG, "TC slip %u selected → CAN 0x%03X", idx, CONFIG_TC_CAN_TX_TC_ID);
}
static void enc2_sw_timer_cb(void *a) { enc2_sw_stable_cb(gpio_get_level(s_enc2_sw.gpio)); }

/* ── GPIO + debounce init helper ─────────────────────────────────────── */
static esp_err_t setup_debounced(debounced_gpio_t *d, gpio_num_t gpio,
                                 esp_timer_cb_t timer_cb, bool pull_up)
{
    d->gpio = gpio;
    gpio_config_t io = {
        .pin_bit_mask  = 1ULL << gpio,
        .mode          = GPIO_MODE_INPUT,
        .pull_up_en    = pull_up ? GPIO_PULLUP_ENABLE  : GPIO_PULLUP_DISABLE,
        .pull_down_en  = pull_up ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type     = GPIO_INTR_ANYEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config %d", gpio);

    esp_timer_create_args_t ta = {
        .callback        = timer_cb,
        .arg             = d,
        .name            = "dbnc",
        .dispatch_method = ESP_TIMER_TASK,
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&ta, &d->timer), TAG, "timer %d", gpio);
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(gpio, debounce_isr, d), TAG, "isr %d", gpio);
    return ESP_OK;
}

/* ── Public init ─────────────────────────────────────────────────────── */
esp_err_t inputs_init(void)
{
    ESP_LOGI(TAG, "inputs init — enc1 A=%d B=%d SW=%d | enc2 A=%d B=%d SW=%d | odo=%d",
             CONFIG_TC_ENC1_A_GPIO, CONFIG_TC_ENC1_B_GPIO, CONFIG_TC_ENC1_SW_GPIO,
             CONFIG_TC_ENC2_A_GPIO, CONFIG_TC_ENC2_B_GPIO, CONFIG_TC_ENC2_SW_GPIO,
             CONFIG_TC_BUTTON_ODO_GPIO);

    esp_err_t isr_ret = gpio_install_isr_service(0);
    if (isr_ret != ESP_OK && isr_ret != ESP_ERR_INVALID_STATE) {
        ESP_RETURN_ON_ERROR(isr_ret, TAG, "isr_service");
    }

    /* Menu auto-close timer */
    esp_timer_create_args_t mta = {
        .callback = menu_close_cb, .name = "menu_close",
        .dispatch_method = ESP_TIMER_TASK,
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&mta, &s_menu_close_timer), TAG, "menu_timer");

    /* ODO button long-press timer */
    esp_timer_create_args_t ota = {
        .callback = odo_long_cb, .name = "odo_long",
        .dispatch_method = ESP_TIMER_TASK,
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&ota, &s_odo_long_timer), TAG, "odo_timer");

    /* Encoder 1 (Boost) */
    ESP_RETURN_ON_ERROR(setup_debounced(&s_enc1_a,  CONFIG_TC_ENC1_A_GPIO,  enc1_a_timer_cb,  true), TAG, "enc1a");
    ESP_RETURN_ON_ERROR(setup_debounced(&s_enc1_b,  CONFIG_TC_ENC1_B_GPIO,  enc1_b_timer_cb,  true), TAG, "enc1b");
    ESP_RETURN_ON_ERROR(setup_debounced(&s_enc1_sw, CONFIG_TC_ENC1_SW_GPIO, enc1_sw_timer_cb, true), TAG, "enc1sw");

    /* Encoder 2 (TC) */
    ESP_RETURN_ON_ERROR(setup_debounced(&s_enc2_a,  CONFIG_TC_ENC2_A_GPIO,  enc2_a_timer_cb,  true), TAG, "enc2a");
    ESP_RETURN_ON_ERROR(setup_debounced(&s_enc2_b,  CONFIG_TC_ENC2_B_GPIO,  enc2_b_timer_cb,  true), TAG, "enc2b");
    ESP_RETURN_ON_ERROR(setup_debounced(&s_enc2_sw, CONFIG_TC_ENC2_SW_GPIO, enc2_sw_timer_cb, true), TAG, "enc2sw");

    /* ODO button */
#if CONFIG_TC_BUTTON_ODO_GPIO >= 0
    ESP_RETURN_ON_ERROR(setup_debounced(&s_odo_btn, CONFIG_TC_BUTTON_ODO_GPIO, odo_timer_cb, true), TAG, "odo");
#endif

    /* Capture initial encoder A-pin states */
    s_enc1_last_a = gpio_get_level(CONFIG_TC_ENC1_A_GPIO);
    s_enc2_last_a = gpio_get_level(CONFIG_TC_ENC2_A_GPIO);

    return ESP_OK;
}
