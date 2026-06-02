/**
 * ui.h — LVGL UI entry point, center cluster.
 */
#ifndef UI_H
#define UI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create all LVGL objects on `disp`. Call once, inside bsp_lvgl_lock().
 * Starts the boot splash; the live cluster screen fades in after ~2 s.
 */
void ui_init(lv_disp_t *disp);

/**
 * 30 Hz paint tick callback — call via lv_timer_create(ui_paint_tick, 33, NULL).
 * Snapshots g_dash and updates every widget. No LVGL mutex needed (runs in
 * the LVGL task context already).
 */
void ui_paint_tick(lv_timer_t *t);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
