#pragma once
#include "lvgl.h"
#include "dash_data.h"
void ui_shift_leds_create(lv_obj_t *parent);
void ui_shift_leds_update(const dash_data_t *d);
