#pragma once
#include "lvgl.h"
#include "dash_data.h"

/** Shift strobe timing for gear box + shift LEDs. Call once per paint tick. */
void ui_shift_alert_update(const dash_data_t *d);

bool ui_shift_alert_strobe_active(void);
bool ui_shift_alert_flash_on(void);
bool ui_shift_alert_all_red(void);
