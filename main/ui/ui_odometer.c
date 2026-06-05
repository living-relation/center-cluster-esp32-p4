/**
 * ui_odometer.c — odometer / trip rectangle, center cluster.
 *
 * 333×58 rect centered at (cx, cy+207).
 * Mode label: RaceHead 22 px top-left; digits: Aerospace 28 px green.
 * Format: ODO "024731.6" (6 digits + decimal, max 999999.9), TRIP "0024.6".
 */
#include "ui_odometer.h"
#include "center-colors.h"

LV_FONT_DECLARE(racehead_22);
LV_FONT_DECLARE(aerospace_28);

#define CX 400
#define CY 400
#define ODO_W 333
#define ODO_H  58
#define ODO_Y (CY + 207)
#define ODO_DIGIT_PAD  8  /* inset from right edge */

static lv_obj_t *s_rect;
static lv_obj_t *s_mode_lbl;
static lv_obj_t *s_digits_lbl;

static void format_tenths(char *buf, size_t n, float val, int width)
{
    uint32_t tenths_total = (uint32_t)(val * 10.0f + 0.5f);
    if (tenths_total > 9999999U) {
        tenths_total = 9999999U;
    }
    uint32_t whole = tenths_total / 10U;
    uint32_t frac  = tenths_total % 10U;
    lv_snprintf(buf, n, "%0*u.%u", width, (unsigned)whole, (unsigned)frac);
}

void ui_odometer_raise(void)
{
    if (s_rect) {
        lv_obj_move_foreground(s_rect);
    }
    if (s_mode_lbl) {
        lv_obj_move_foreground(s_mode_lbl);
    }
    if (s_digits_lbl) {
        lv_obj_move_foreground(s_digits_lbl);
    }
}

void ui_odometer_create(lv_obj_t *parent)
{
    s_rect = lv_obj_create(parent);
    lv_obj_set_size(s_rect, ODO_W, ODO_H);
    lv_obj_align(s_rect, LV_ALIGN_TOP_MID, 0, ODO_Y - CY + 400 - ODO_H / 2);
    lv_obj_set_style_bg_color(s_rect, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_rect, COLOR_GREEN, 0);
    lv_obj_set_style_border_opa(s_rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_rect, 2, 0);
    lv_obj_set_style_radius(s_rect, 0, 0);
    lv_obj_clear_flag(s_rect, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_rect, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    s_mode_lbl = lv_label_create(s_rect);
    lv_label_set_text(s_mode_lbl, "ODO");
    lv_obj_set_style_text_color(s_mode_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_mode_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_mode_lbl, &racehead_22, 0);
    lv_obj_align(s_mode_lbl, LV_ALIGN_TOP_LEFT, 5, 5);

    s_digits_lbl = lv_label_create(s_rect);
    lv_label_set_text(s_digits_lbl, "000000.0");
    lv_obj_set_style_text_color(s_digits_lbl, COLOR_GREEN, 0);
    lv_obj_set_style_text_opa(s_digits_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_digits_lbl, &aerospace_28, 0);
    lv_obj_set_style_text_align(s_digits_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(s_digits_lbl, ODO_W - (ODO_DIGIT_PAD * 2));
    lv_label_set_long_mode(s_digits_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_align(s_digits_lbl, LV_ALIGN_RIGHT_MID, -ODO_DIGIT_PAD, 4);
}

void ui_odometer_update(const dash_data_t *d)
{
    const char *mode_str = "ODO";
    float val = d->odo;
    if (d->odo_mode == DASH_TRIP_A) { mode_str = "TRIP A"; val = d->trip_a; }
    if (d->odo_mode == DASH_TRIP_B) { mode_str = "TRIP B"; val = d->trip_b; }
    lv_label_set_text(s_mode_lbl, mode_str);

    char digits[16];
    if (d->odo_mode == DASH_ODO) {
        format_tenths(digits, sizeof(digits), val, 6);
    } else {
        format_tenths(digits, sizeof(digits), val, 6);
    }
    lv_label_set_text(s_digits_lbl, digits);
}
