/**
 * ui_shift_leds.c — 10-LED shift column, center cluster.
 *
 * 10 × lv_obj rects, 24×27 px, corner radius 7, gap 7, black outline.
 * Column x = cx + 262. Top = cy − 167. Drawn bottom-up.
 *
 * Strobe timing from ui_shift_alert.c (shared with gear box, ≥6000 RPM).
 */
#include "ui_shift_leds.h"
#include "ui_shift_alert.h"
#include "center-colors.h"

#define LED_W     24
#define LED_H     27
#define LED_GAP    7
#define LED_R      7
#define LED_COL  (400 + 262)
#define LED_TOP  (400 - 167)

#define N_LEDS   10

static lv_obj_t *s_leds[N_LEDS];

void ui_shift_leds_create(lv_obj_t *parent)
{
    for (int i = 0; i < N_LEDS; i++) {
        int y = LED_TOP + (N_LEDS - 1 - i) * (LED_H + LED_GAP);
        lv_obj_t *led = lv_obj_create(parent);
        lv_obj_set_size(led, LED_W, LED_H);
        lv_obj_set_pos(led, LED_COL - LED_W / 2, y);
        lv_obj_set_style_radius(led, LED_R, 0);
        lv_obj_set_style_bg_color(led, COLOR_INACTIVE, 0);
        lv_obj_set_style_bg_opa(led, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(led, 2, 0);
        lv_obj_set_style_border_color(led, COLOR_BG_PRIMARY, 0);
        lv_obj_set_style_border_opa(led, LV_OPA_COVER, 0);
        lv_obj_clear_flag(led, LV_OBJ_FLAG_SCROLLABLE);
        s_leds[i] = led;
    }
}

void ui_shift_leds_update(const dash_data_t *d)
{
    bool flash_on = ui_shift_alert_flash_on();

    int lit = 0;
    if (d->rpm >= 2000.0f) {
        lit = (int)((d->rpm - 2000.0f) / ((7500.0f - 2000.0f) / N_LEDS));
    }
    if (lit > N_LEDS) {
        lit = N_LEDS;
    }

    for (int i = 0; i < N_LEDS; i++) {
        bool is_lit = (i < lit) && flash_on;
        lv_color_t col = is_lit ? shift_led_color(i) : COLOR_INACTIVE;
        lv_obj_set_style_bg_color(s_leds[i], col, 0);
        lv_obj_set_style_bg_opa(s_leds[i], is_lit ? LV_OPA_COVER : LV_OPA_20, 0);
    }
}
