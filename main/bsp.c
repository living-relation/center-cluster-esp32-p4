/**
 * bsp.c — Waveshare ESP32-P4-WIFI6-Touch-LCD-XC board support.
 *
 * Display: MIPI-DSI 2-lane to a round LCD (800×800).
 * Schematic-verified pin assignments (Rev 1.1):
 *   GPIO 6  = ESP_I2C_SDA  (touch, camera, shared bus)
 *   GPIO 7  = ESP_I2C_SCL
 *   GPIO 26 = LCD backlight (LEDC PWM, output_invert=1 per Waveshare BSP)
 *   GPIO 27 = LCD RESET (active-low)
 *   DSI data/clock = dedicated MIPI pads (not GPIO-numbered)
 *
 * Backlight: driven by firmware on GPIO 26 with LEDC (output_invert=1), so a
 * duty of 1023 = full brightness. Without this init the boost converter never
 * enables and the LCD stays dark even though DSI/LVGL render correctly.
 */

#include "bsp.h"
#include "sdkconfig.h"

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_jd9365.h"     /* espressif/esp_lcd_jd9365 — JD9365 panel driver with full
                                    init sequence; required for the integrated
                                    ESP32-P4-WIFI6-Touch-LCD-3.4C (the previously used
                                    waveshare/esp_lcd_dsi driver targets the standalone
                                    Waveshare adapter board and leaves the panel blank). */
#include "jd9365_init_3_4_inch_c.h"  /* Panel-specific JD9365 init sequence for the
                                         800x800 round 3.4" panel (Waveshare variant C).
                                         The default sequence inside esp_lcd_jd9365 is
                                         for the 800x1280 panel and will not bring up
                                         this round panel. */
#include "esp_ldo_regulator.h"
#include "esp_lvgl_port.h"

static const char *TAG = "bsp";

/* ── Board-fixed GPIO assignments (schematic Rev 1.1) ─────────────────── */
#define BSP_LCD_BL_GPIO      26
#define BSP_LCD_RESET_GPIO   27
#define BSP_I2C_SCL_GPIO      7
#define BSP_I2C_SDA_GPIO      6
#define BSP_DSI_LANE_MBPS  1250   /* MIPI bit-rate per lane (Waveshare 3.4" panel default) */

/* ── Backlight (LEDC PWM, matches official Waveshare esp32_p4_wifi6_touch_lcd_xc BSP) ── */
#define BSP_BL_DUTY_MAX  1023u

static bool s_backlight_pwm_ready = false;

void bsp_backlight_hold_off(void)
{
    gpio_config_t bl = {
        .pin_bit_mask = 1ULL << BSP_LCD_BL_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl);
    gpio_set_level(BSP_LCD_BL_GPIO, 0);
}

static esp_err_t backlight_pwm_init_off(void)
{
    if (s_backlight_pwm_ready) {
        return ESP_OK;
    }

    const ledc_timer_config_t bl_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = LEDC_TIMER_1,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&bl_timer), TAG, "bl_timer");

    const ledc_channel_config_t bl_ch = {
        .gpio_num   = BSP_LCD_BL_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,              /* output_invert=1 → duty 0 keeps backlight off */
        .hpoint     = 0,
        .flags      = { .output_invert = 1 },
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&bl_ch), TAG, "bl_channel");
    s_backlight_pwm_ready = true;
    return ESP_OK;
}

void bsp_backlight_on(void)
{
    ESP_ERROR_CHECK(backlight_pwm_init_off());
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, BSP_BL_DUTY_MAX));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
    ESP_LOGI(TAG, "Backlight on");
}

void bsp_backlight_set_percent(uint8_t pct)
{
    /* Pure hardware setter: 0..100 % → LEDC duty. output_invert=1 already makes
     * a higher percent brighter. Brightness policy (day/night, floor) lives in
     * the caller (inputs.c); this only guards the raw 0..100 range. */
    ESP_ERROR_CHECK(backlight_pwm_init_off());
    if (pct > 100u) pct = 100u;
    uint32_t duty = (uint32_t)BSP_BL_DUTY_MAX * pct / 100u;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

/* ── MIPI-DSI PHY LDO ────────────────────────────────────────────────── */
static esp_ldo_channel_handle_t s_ldo_mipi = NULL;

static esp_err_t mipi_phy_power_on(void)
{
    /* The MIPI-DSI PHY on ESP32-P4 needs LDO channel 3 at 2.5 V. */
    esp_ldo_channel_config_t cfg = {
        .chan_id    = 3,
        .voltage_mv = 2500,
    };
    return esp_ldo_acquire_channel(&cfg, &s_ldo_mipi);
}

/* ── Panel handles ───────────────────────────────────────────────────── */
static esp_lcd_panel_handle_t    s_panel   = NULL;
static esp_lcd_panel_io_handle_t s_io      = NULL;
static esp_lcd_dsi_bus_handle_t  s_dsi_bus = NULL;

static void panel_framebuffer_black(void)
{
    void *fb0 = NULL;
    void *fb1 = NULL;
    const size_t fb_bytes = (size_t)BSP_LCD_H_RES * BSP_LCD_V_RES * 2;
    esp_err_t err = esp_lcd_dpi_panel_get_frame_buffer(s_panel, 2, &fb0, &fb1);
    if (err == ESP_OK && fb0 != NULL) {
        memset(fb0, 0, fb_bytes);
        if (fb1 != NULL) {
            memset(fb1, 0, fb_bytes);
        }
        return;
    }
    fb0 = NULL;
    err = esp_lcd_dpi_panel_get_frame_buffer(s_panel, 1, &fb0);
    if (err != ESP_OK || fb0 == NULL) {
        ESP_LOGW(TAG, "DSI FB clear skipped (%s)", esp_err_to_name(err));
        return;
    }
    memset(fb0, 0, fb_bytes);
}

static esp_err_t panel_init(void)
{
    /* 1. DSI bus (2 data lanes) */
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id             = 0,
        .num_data_lanes     = 2,
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = BSP_DSI_LANE_MBPS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &s_dsi_bus), TAG, "dsi_bus");

    /* 2. DBI command channel (panel init over MIPI DCS) */
    esp_lcd_dbi_io_config_t dbi_cfg = {
        .virtual_channel = 0,
        .lcd_cmd_bits    = 8,
        .lcd_param_bits  = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(s_dsi_bus, &dbi_cfg, &s_io), TAG, "dbi_io");

    /* 3. DPI video timing — official Waveshare/Espressif BSP values for the
     *    integrated 800×800 round 3.4C panel (JD9365 controller). */
    esp_lcd_dpi_panel_config_t dpi_cfg = {
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        /* 800x800 timing totals: (800+20+20+40) x (800+12+4+24) = 739200 px/frame.
         * 45 MHz pixel clock yields ~60.9 Hz scanout, which is the highest stable
         * practical refresh for this panel with minimal tearing. */
        .dpi_clock_freq_mhz = 45,
        .virtual_channel    = 0,
        .pixel_format       = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .num_fbs            = 2,   /* two PSRAM framebuffers owned by the DSI
                                      controller — enables lvgl_port direct-mode
                                      (zero-copy, hardware-driven refresh, no
                                      per-frame CPU blit, no tearing). */
        .video_timing = {
            .h_size            = BSP_LCD_H_RES,
            .v_size            = BSP_LCD_V_RES,
            .hsync_back_porch  = 20,
            .hsync_pulse_width = 20,
            .hsync_front_porch = 40,
            .vsync_back_porch  = 12,
            .vsync_pulse_width = 4,
            .vsync_front_porch = 24,
        },
        .flags.use_dma2d = true,
    };

    /* 4. JD9365 panel via the espressif/esp_lcd_jd9365 driver. We pass the
     *    panel-specific init sequence for the 3.4" round 800x800 variant
     *    (variant C) because the driver's built-in default targets a different
     *    800x1280 JD9365 panel and leaves this one blank. */
    jd9365_vendor_config_t vendor_cfg = {
        .init_cmds      = jd9365_3_4_inch_c_init_cmds,
        .init_cmds_size = sizeof(jd9365_3_4_inch_c_init_cmds) / sizeof(jd9365_3_4_inch_c_init_cmds[0]),
        .mipi_config = {
            .dsi_bus    = s_dsi_bus,
            .dpi_config = &dpi_cfg,
            .lane_num   = 2,
        },
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BSP_LCD_RESET_GPIO,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor_cfg,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_jd9365(s_io, &panel_cfg, &s_panel), TAG, "jd9365_panel");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel),  TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "disp_on");
    return ESP_OK;
}

/* ── Public API ──────────────────────────────────────────────────────── */
esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "TrackCluster Center BSP — ESP32-P4-WIFI6-Touch-LCD-XC");
    bsp_backlight_hold_off();
    ESP_RETURN_ON_ERROR(mipi_phy_power_on(), TAG, "mipi_ldo");
    ESP_RETURN_ON_ERROR(panel_init(),        TAG, "panel");
    ESP_RETURN_ON_ERROR(backlight_pwm_init_off(), TAG, "backlight_pwm");
    return ESP_OK;
}

/* ── LVGL bring-up ───────────────────────────────────────────────────── */
static lv_disp_t *s_disp = NULL;

lv_disp_t *bsp_display_start(void)
{
    if (s_disp) return s_disp;

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_affinity = 1;  /* Keep LVGL on one core for steadier frame pacing. */
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = s_io,
        .panel_handle  = s_panel,
        .buffer_size   = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .hres          = BSP_LCD_H_RES,
        .vres          = BSP_LCD_V_RES,
        .monochrome    = false,
        .color_format  = LV_COLOR_FORMAT_RGB565,
        .rotation      = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        /* swap_bytes MUST be false: the ESP32-P4 DSI controller emits native
         * RGB565 byte order. Enabling swap_bytes inverts the color channels
         * (purple skies, broken anti-aliased fonts) and forces a CPU pass over
         * every pixel each frame. */
        .flags         = {
            .buff_dma = false,
            .buff_spiram = true,
            .swap_bytes = false,
            .sw_rotate = true,
            .direct_mode = true,
        },
    };
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = { .avoid_tearing = true },
    };
    s_disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);

    if (bsp_lvgl_lock(portMAX_DELAY)) {
        lv_obj_t *scr = lv_screen_active();
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        lv_obj_invalidate(scr);
        lv_refr_now(NULL);
        lv_refr_now(NULL);
        bsp_lvgl_unlock();
    }
    panel_framebuffer_black();

    return s_disp;
}

bool bsp_lvgl_lock(uint32_t timeout_ms) { return lvgl_port_lock(timeout_ms); }
void bsp_lvgl_unlock(void)              {        lvgl_port_unlock();          }
