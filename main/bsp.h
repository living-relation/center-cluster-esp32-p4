/**
 * bsp.h — board support for Waveshare ESP32-P4-WIFI6-Touch-LCD-XC.
 *
 * Backlight note: NOT controlled by firmware. The BL_PWM brightness pin and
 * the BL_EN boost-converter enable are both handled in hardware (external PWM
 * controller; BL_EN tied high via R66 on the board). No backlight-enable or
 * dimming code in this BSP.
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
