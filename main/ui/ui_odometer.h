#pragma once
#include "lvgl.h"
#include "dash_data.h"
void ui_odometer_create(lv_obj_t *parent);
void ui_odometer_raise(void);
void ui_odometer_update(const dash_data_t *d);
