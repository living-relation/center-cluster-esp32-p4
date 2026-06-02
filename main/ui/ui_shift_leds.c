/**
 * ui_shift_leds.c — 10-LED shift column, center cluster.
 *
 * 10 × lv_obj rects, 24×27 px, corner radius 7, gap 7.
 * Column x = cx + 262. Top = cy − 167. Drawn bottom-up.
 *
 * All-red hysteresis: latches at RPM ≥ 7000, releases below 6500.
 * Flash: slow (220 ms) ≥ 5500, fast (80 ms) ≥ 6500.
 * See center-07-state-machines.md for full state machine.
 */
#include "ui_shift_leds.h"
#include "center-colors.h"

#define LED_W     24
#define LED_H     27
#define LED_GAP    7
#define LED_R      7
#define LED_COL  (400 + 262)
#define LED_TOP  (400 - 167)

#define N_LEDS   10

static lv_obj_t *s_leds[N_LEDS];
static bool      s_all_red = false;
static bool      s_flash_on = true;
static uint32_t  s_last_toggle_ms = 0;

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
        lv_obj_set_style_border_width(led, 0, 0);
        lv_obj_clear_flag(led, LV_OBJ_FLAG_SCROLLABLE);
        s_leds[i] = led;
    }
}

void ui_shift_leds_update(const dash_data_t *d)
{
    /* All-red hysteresis */
    if (!s_all_red && d->rpm >= DASH_RPM_LEDS_ALL_RED_ON)  s_all_red = true;
    if ( s_all_red && d->rpm <  DASH_RPM_LEDS_ALL_RED_OFF) s_all_red = false;

    /* Flash timer */
    uint32_t now = lv_tick_get();
    uint32_t period = (d->rpm >= DASH_RPM_SHIFT_FAST) ? 80u : 220u;
    if (d->rpm >= DASH_RPM_SHIFT_SLOW) {
        if ((now - s_last_toggle_ms) >= period) {
            s_flash_on = !s_flash_on;
            s_last_toggle_ms = now;
        }
    } else {
        s_flash_on = true;
    }

    /* Lit count */
    int lit = 0;
    if (d->rpm >= 2000.0f)
        lit = (int)((d->rpm - 2000.0f) / ((7500.0f - 2000.0f) / N_LEDS));
    if (lit > N_LEDS) lit = N_LEDS;

    for (int i = 0; i < N_LEDS; i++) {
        bool is_lit = (i < lit) && s_flash_on;
        lv_color_t col = is_lit ? shift_led_color(i, s_all_red) : COLOR_INACTIVE;
        lv_obj_set_style_bg_color(s_leds[i], col, 0);
        lv_obj_set_style_bg_opa(s_leds[i],
            is_lit ? LV_OPA_COVER : LV_OPA_20, 0);
    }
}
