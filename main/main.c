/**
 * main.c — center cluster application entry point.
 *
 * Boot sequence:
 *   1. nvs_flash_init()    — odometer persistence
 *   2. bsp_init()          — MIPI-DSI panel bring-up
 *   3. bsp_display_start() — LVGL port + display registration
 *   4. ui_init()           — LVGL widget tree (boot splash → live screen)
 *   5. NORMAL: inputs_init() + canbus_task + uart_tx_task   (real hardware)
 *      BENCH:  bench_task (CONFIG_TC_BENCH_MODE — standalone demo sweep)
 *   6. alarm_task          — 20 ms threshold scanner → g_dash.flags (both modes)
 *
 * The center has no "no-data" overlay: with nothing connected the gauges simply
 * render g_dash at 0. Bench mode sweeps them so every widget can be verified.
 */

#include "bsp.h"
#include "canbus.h"
#include "uart_bridge.h"
#include "inputs.h"
#include "dash_data.h"
#include "ui/ui.h"
#include "audio_alert.h"
#include "can_sim.h"
#include "sdkconfig.h"

#if CONFIG_TC_CAN_SIM_CONSOLE
#include "esp_console.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <math.h>

static const char *TAG = "main";

#define ODO_NVS_NS          "tc"
#define ODO_NVS_KEY_TENTHS  "odo10"
#define ODO_START_MILES     100.0f

extern portMUX_TYPE g_dash_mux;

static void odometer_init(void)
{
    uint32_t tenths = (uint32_t)(ODO_START_MILES * 10.0f + 0.5f);
    nvs_handle_t h;
    esp_err_t err = nvs_open(ODO_NVS_NS, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        uint32_t stored = 0;
        err = nvs_get_u32(h, ODO_NVS_KEY_TENTHS, &stored);
        if (err == ESP_ERR_NVS_NOT_FOUND || stored == 0) {
            stored = tenths;
            ESP_ERROR_CHECK(nvs_set_u32(h, ODO_NVS_KEY_TENTHS, stored));
            ESP_ERROR_CHECK(nvs_commit(h));
            ESP_LOGI(TAG, "ODO seeded to %.1f mi", ODO_START_MILES);
        } else if (err == ESP_OK) {
            tenths = stored;
        } else {
            ESP_LOGW(TAG, "ODO NVS read failed (%s) — using %.1f mi",
                     esp_err_to_name(err), ODO_START_MILES);
        }
        nvs_close(h);
    } else {
        ESP_LOGW(TAG, "ODO NVS open failed (%s) — using %.1f mi",
                 esp_err_to_name(err), ODO_START_MILES);
    }

    portENTER_CRITICAL(&g_dash_mux);
    g_dash.odo      = (float)tenths / 10.0f;
    g_dash.odo_mode = DASH_ODO;
    portEXIT_CRITICAL(&g_dash_mux);
}

extern void alarm_task(void *arg);

#if CONFIG_TC_CAN_SIM_CONSOLE
#include <stdio.h>
#include <string.h>

static void can_sim_stdin_task(void *arg)
{
    (void)arg;
    char line[256];
    for (;;) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) {
            continue;
        }
        int ret = 0;
        esp_console_run(line, &ret);
        fflush(stdout);
    }
}

static void can_sim_console_start(void)
{
    const esp_console_config_t cfg = ESP_CONSOLE_CONFIG_DEFAULT();
    esp_err_t err = esp_console_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "console init failed (%s)", esp_err_to_name(err));
        return;
    }
    err = can_sim_console_register();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "can command register failed (%s)", esp_err_to_name(err));
        return;
    }
    xTaskCreate(can_sim_stdin_task, "can_cli", 4096, NULL, 3, NULL);
    printf("CAN sim ready — warn_test | can 3ee 0100000000000000\n");
    fflush(stdout);
}

#endif

#if CONFIG_TC_BENCH_MODE
#define BENCH_RPM_MAX        8000.0f
#define BENCH_TICK_MS        4U
#define BENCH_SWEEP_RPM_S    4000.0f
#define BENCH_RPM_STEP       (BENCH_SWEEP_RPM_S * ((float)BENCH_TICK_MS / 1000.0f))

typedef struct {
    float    target_rpm;
    uint32_t hold_ms;
} bench_leg_t;

/* Scripted sweep so key RPM points (5k/6k/6.5k/7k/max) can be inspected. */
static const bench_leg_t BENCH_LEGS[] = {
    { 8000.0f, 2000 },
    { 5000.0f, 1000 },
    { 8000.0f, 1000 },
    { 6000.0f, 1000 },
    { 8000.0f, 1000 },
    { 6500.0f, 1000 },
    { 8000.0f,    0 },
    { 7000.0f, 1000 },
    { 8000.0f,    0 },
    {    0.0f,    0 },
};
#define BENCH_LEGS_N ((int)(sizeof(BENCH_LEGS) / sizeof(BENCH_LEGS[0])))

static void bench_task(void *arg);

void app_bench_start(void)
{
    ESP_LOGW(TAG, "BENCH MODE — starting scripted sweep");
    xTaskCreatePinnedToCore(bench_task, "bench", 4096, NULL, 4, NULL, 0);
}

static void bench_task(void *arg)
{
    const TickType_t bench_period = pdMS_TO_TICKS(BENCH_TICK_MS);
    const uint32_t min_shift_interval_ms = 280;

    TickType_t wake = xTaskGetTickCount();
    float rpm = 0.0f;
    int leg = 0;
    uint32_t hold_remain_ms = 0;
    int8_t gear = 1;
    uint32_t last_shift_ms = 0;

    portENTER_CRITICAL(&g_dash_mux);
    g_dash.odo      = 100.0f;
    g_dash.odo_mode = DASH_ODO;
    portEXIT_CRITICAL(&g_dash_mux);

    for (;;) {
        if (hold_remain_ms > 0) {
            if (hold_remain_ms > BENCH_TICK_MS) {
                hold_remain_ms -= BENCH_TICK_MS;
            } else {
                hold_remain_ms = 0;
            }
        } else {
            float target = BENCH_LEGS[leg].target_rpm;
            float delta = target - rpm;
            if (fabsf(delta) <= BENCH_RPM_STEP) {
                rpm = target;
                hold_remain_ms = BENCH_LEGS[leg].hold_ms;
                leg++;
                if (leg >= BENCH_LEGS_N) {
                    leg = 0;
                }
            } else if (delta > 0.0f) {
                rpm += BENCH_RPM_STEP;
                if (rpm > target) {
                    rpm = target;
                }
            } else {
                rpm -= BENCH_RPM_STEP;
                if (rpm < target) {
                    rpm = target;
                }
            }
        }

        const float mph = (rpm / BENCH_RPM_MAX) * 120.0f;
        const uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if ((now_ms - last_shift_ms) >= min_shift_interval_ms) {
            if (gear < 6 && rpm > 6400.0f) {
                gear++;
                last_shift_ms = now_ms;
            } else if (gear > 1 && rpm < 2200.0f) {
                gear--;
                last_shift_ms = now_ms;
            }
        }

        portENTER_CRITICAL(&g_dash_mux);
        g_dash.rpm            = rpm;
        g_dash.mph            = mph;
        g_dash.gear           = gear;
        g_dash.last_update_ms = now_ms;
        portEXIT_CRITICAL(&g_dash_mux);

        vTaskDelayUntil(&wake, bench_period);
    }
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "TrackCluster Center — booting");

    /* NVS (odometer persistence) */
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition erased — odometer reset");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);
    odometer_init();

    /* Board bring-up */
    bsp_backlight_hold_off();
    ESP_ERROR_CHECK(bsp_init());
    lv_disp_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "LVGL display init failed — halting");
        for (;;) vTaskDelay(portMAX_DELAY);
    }

    /* UI */
    bsp_lvgl_lock(portMAX_DELAY);
    ui_init(disp);
    bsp_lvgl_unlock();

#if CONFIG_TC_BENCH_MODE
    ESP_LOGW(TAG, "BENCH MODE — sweep starts after boot splash");
#else
    /* Inputs: ODO button + dual rotary encoders (GPIO ISRs + esp_timer debounce) */
    ESP_ERROR_CHECK(inputs_init());
    xTaskCreatePinnedToCore(canbus_task,  "can",   8192, NULL, 7, NULL, 0);
    xTaskCreatePinnedToCore(uart_tx_task, "uart",  4096, NULL, 6, NULL, 0);
#endif
    xTaskCreatePinnedToCore(alarm_task,   "alarm", 4096, NULL, 6, NULL, 0);

    /* Always on: warning tone when ECU 0x3EE sets g_dash.warn (rising edge). */
    audio_alert_warn_monitor_start();
#if CONFIG_TC_CAN_SIM_CONSOLE
    can_sim_console_start();
#endif

    ESP_LOGI(TAG, "All tasks started");
}
