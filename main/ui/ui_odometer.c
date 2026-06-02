/**
 * ui_odometer.c — odometer / trip rectangle, center cluster.
 *
 * 333×58 rect centered at (cx, cy+207).
 * Mode label: RaceHead 14 px top-left; digits: Aerospace 29 px green 85%.
 * Format: ODO "00024731.6" (9 chars + decimal), TRIP "0024.6" (6 chars).
 * Persisted to NVS every 60 s and on brown-out hook.
 */
#include "ui_odometer.h"
#include "center-colors.h"

LV_FONT_DECLARE(racehead_14);
LV_FONT_DECLARE(racehead_22);
LV_FONT_DECLARE(aerospace_29);

#define CX 400
#define CY 400
#define ODO_W 333
#define ODO_H  58
#define ODO_Y (CY + 207)

static lv_obj_t *s_rect;
static lv_obj_t *s_mode_lbl;
static lv_obj_t *s_digits_lbl;

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

    s_mode_lbl = lv_label_create(s_rect);
    lv_label_set_text(s_mode_lbl, "ODO");
    lv_obj_set_style_text_color(s_mode_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_mode_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_mode_lbl, &racehead_22, 0);
    lv_obj_align(s_mode_lbl, LV_ALIGN_TOP_LEFT, 5, 5);

    s_digits_lbl = lv_label_create(s_rect);
    lv_label_set_text(s_digits_lbl, "00000000.0");
    lv_obj_set_style_text_color(s_digits_lbl, COLOR_GREEN, 0);
    lv_obj_set_style_text_opa(s_digits_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_digits_lbl, &aerospace_29, 0);
    lv_obj_align(s_digits_lbl, LV_ALIGN_CENTER, 0, 4);
}

void ui_odometer_update(const dash_data_t *d)
{
    const char *mode_str = "ODO";
    float val = d->odo;
    if (d->odo_mode == DASH_TRIP_A) { mode_str = "TRIP A"; val = d->trip_a; }
    if (d->odo_mode == DASH_TRIP_B) { mode_str = "TRIP B"; val = d->trip_b; }
    lv_label_set_text(s_mode_lbl, mode_str);
    if (d->odo_mode == DASH_ODO)
        lv_label_set_text_fmt(s_digits_lbl, "%09.1f", (double)val);
    else
        lv_label_set_text_fmt(s_digits_lbl, "%06.1f", (double)val);
}
