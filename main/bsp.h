/**
 * bsp.h — board support for Waveshare ESP32-P4-WIFI6-Touch-LCD-XC.
 *
 * Backlight: controlled by firmware on GPIO 26 via LEDC PWM with
 * output_invert=1 (matches Waveshare's official BSP). Without this the
 * boost converter never enables and the LCD stays dark.
 */
#ifndef BSP_H
#define BSP_H

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_LCD_H_RES   800
#define BSP_LCD_V_RES   800

/** Force backlight off before panel/LVGL init (avoids white flash at power-on). */
void bsp_backlight_hold_off(void);

/** Enable backlight after the first black LVGL frame (call from ui_init). */
void bsp_backlight_on(void);

/** One-time board bring-up. Call before bsp_display_start(). */
esp_err_t  bsp_init(void);

/** Create the LVGL display. Returns the display object. */
lv_disp_t *bsp_display_start(void);

/** Acquire / release the LVGL mutex (wraps esp_lvgl_port). */
bool bsp_lvgl_lock(uint32_t timeout_ms);
void bsp_lvgl_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */
