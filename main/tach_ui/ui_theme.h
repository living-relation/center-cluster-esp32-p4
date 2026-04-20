#ifndef _STIMETER_UI_THEME_H
#define _STIMETER_UI_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef enum {
    UI_THEME_SPORT = 0,
    UI_THEME_OEM = 1,
    UI_THEME_STEALTH = 2,
    UI_THEME_COUNT
} ui_theme_t;

typedef struct {
    lv_color_t screen_bg_color;
    lv_color_t rpm_color_low;
    lv_color_t rpm_color_mid;
    lv_color_t rpm_color_high;
    lv_color_t mph_value_color;
    lv_color_t primary_label_color;
    lv_color_t gear_value_color;
    lv_color_t odometer_color;
    lv_opa_t rpm_arc_opa;
    int16_t rpm_arc_width;
    lv_opa_t rpm_bg_img_opa;
    int16_t rpm_glow_width;
    lv_opa_t rpm_glow_opa;
} ui_theme_palette_t;

#ifndef UI_THEME_DEFAULT
#define UI_THEME_DEFAULT UI_THEME_SPORT
#endif

void ui_theme_set(ui_theme_t theme);
ui_theme_t ui_theme_get(void);
const char *ui_theme_name(ui_theme_t theme);
const ui_theme_palette_t *ui_theme_get_palette(ui_theme_t theme);
const ui_theme_palette_t *ui_theme_current_palette(void);
lv_color_t ui_theme_rpm_color_low(void);
lv_color_t ui_theme_rpm_color_mid(void);
lv_color_t ui_theme_rpm_color_high(void);
lv_color_t ui_theme_rpm_color_for_value(int rpm);
lv_color_t ui_theme_mph_value_color(void);
lv_color_t ui_theme_label_color(void);
lv_color_t ui_theme_gear_value_color(void);
lv_color_t ui_theme_odometer_color(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
