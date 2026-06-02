#pragma once
#include "lvgl.h"
typedef void (*ui_boot_done_cb_t)(void);
void ui_boot_start(lv_obj_t *parent, ui_boot_done_cb_t on_done);
