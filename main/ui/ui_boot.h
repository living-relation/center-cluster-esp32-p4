#pragma once
#include "lvgl.h"

/* Splash timeline (keep in sync with ui_boot.c). */
#define UI_BOOT_BLACK_HOLD_MS  150
#define UI_BOOT_FADE_IN_MS     1200
#define UI_BOOT_HOLD_MS        5000
#define UI_BOOT_FADE_OUT_MS    1200
#define UI_BOOT_CHIME_START_MS (UI_BOOT_BLACK_HOLD_MS + UI_BOOT_FADE_IN_MS + (UI_BOOT_HOLD_MS / 2))
#define UI_BOOT_SPLASH_END_MS  (UI_BOOT_BLACK_HOLD_MS + UI_BOOT_FADE_IN_MS + UI_BOOT_HOLD_MS + UI_BOOT_FADE_OUT_MS)

typedef void (*ui_boot_done_cb_t)(void);
void ui_boot_start(lv_obj_t *parent, ui_boot_done_cb_t on_done);
