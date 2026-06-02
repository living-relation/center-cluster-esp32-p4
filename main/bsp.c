/**
 * bsp.c — Waveshare ESP32-P4-WIFI6-Touch-LCD-XC board support.
 *
 * Display: MIPI-DSI 2-lane to a round LCD (800×800).
 * Schematic-verified pin assignments (Rev 1.1):
 *   GPIO 6  = ESP_I2C_SDA  (touch, camera, shared bus)
 *   GPIO 7  = ESP_I2C_SCL
 *   GPIO 27 = LCD RESET (active-low)
 *   DSI data/clock = dedicated MIPI pads (not GPIO-numbered)
 *
 * Backlight: NOT controlled by firmware. Both the BL_PWM brightness pin and
 * the BL_EN boost-converter enable are handled entirely in hardware (external
 * PWM controller; BL_EN tied high via R66). There is no backlight-enable or
 * headlight-dimming code in this BSP.
 */

#include "bsp.h"
#include "sdkconfig.h"

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_lvgl_port.h"

static const char *TAG = "bsp";

/* ── Board-fixed GPIO assignments (schematic Rev 1.1) ─────────────────── */
#define BSP_LCD_RESET_GPIO   27
#define BSP_I2C_SCL_GPIO      7
#define BSP_I2C_SDA_GPIO      6
#define BSP_DSI_LANE_MBPS  1000   /* MIPI bit-rate per lane */

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

static esp_err_t panel_init(void)
{
    /* 1. DSI bus */
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

    /* 3. DPI video timing — matches the 800×800 round panel on this board.
     *    Adjust if Waveshare ships a different panel on your SKU. */
    esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 40,
        .pixel_format       = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .num_fbs            = 1,
        .video_timing = {
            .h_size            = BSP_LCD_H_RES,
            .v_size            = BSP_LCD_V_RES,
            .hsync_pulse_width = 20,
            .hsync_back_porch  = 20,
            .hsync_front_porch = 40,
            .vsync_pulse_width = 4,
            .vsync_back_porch  = 12,
            .vsync_front_porch = 24,
        },
        .flags.use_dma2d = true,
    };

    /* Use esp_lcd_gc9503 if available (preferred for this board); otherwise
     * fall back to the generic DPI panel driver. The managed component is
     * declared in main/idf_component.yml. */
#if __has_include("esp_lcd_gc9503.h")
    #include "esp_lcd_gc9503.h"
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BSP_LCD_RESET_GPIO,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &dpi_cfg,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_gc9503(s_io, &panel_cfg, &s_panel), TAG, "gc9503");
#else
    ESP_LOGW(TAG, "esp_lcd_gc9503 not found — using generic DPI panel driver");
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_dpi(s_dsi_bus, &dpi_cfg, &s_panel), TAG, "dpi_panel");
#endif

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel),          TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel),           TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "disp_on");
    return ESP_OK;
}

/* ── Public API ──────────────────────────────────────────────────────── */
esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "TrackCluster Center BSP — ESP32-P4-WIFI6-Touch-LCD-XC");
    ESP_RETURN_ON_ERROR(mipi_phy_power_on(), TAG, "mipi_ldo");
    ESP_RETURN_ON_ERROR(panel_init(),        TAG, "panel");
    return ESP_OK;
}

/* ── LVGL bring-up ───────────────────────────────────────────────────── */
static lv_disp_t *s_disp = NULL;

lv_disp_t *bsp_display_start(void)
{
    if (s_disp) return s_disp;

    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = s_io,
        .panel_handle  = s_panel,
        .buffer_size   = BSP_LCD_H_RES * 80,   /* 80-line PSRAM slice */
        .double_buffer = true,
        .hres          = BSP_LCD_H_RES,
        .vres          = BSP_LCD_V_RES,
        .monochrome    = false,
        .color_format  = LV_COLOR_FORMAT_RGB565,
        .rotation      = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags         = { .buff_dma = false, .buff_spiram = true, .swap_bytes = true },
    };
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = { .avoid_tearing = false },
    };
    s_disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    return s_disp;
}

bool bsp_lvgl_lock(uint32_t timeout_ms) { return lvgl_port_lock(timeout_ms); }
void bsp_lvgl_unlock(void)              {        lvgl_port_unlock();          }
