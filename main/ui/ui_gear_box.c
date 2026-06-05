/**
 * ui_gear_box.c — gear box + gear glyph + RPM digital readout, center cluster.
 *
 * Gear box: 244×244 centered rect. Bg tint ramps with RPM:
 *   0–2999    → transparent    3000–4000 → GREEN (0.08–0.62 opacity)
 *   4001–6400 → GOLD           6401+     → RED_HOT
 *   ≥6000 RPM → background strobe (rate scales to ~20 Hz at 8000, shared w/ LEDs)
 *
 * Gear glyph: Aerospace 188 px, always white.
 * RPM digital: Aerospace 87 px, color tracks segment color.
 * RPM label:   RaceHead 22 px "RPM", white 32% alpha, below digital.
 */
#include "ui_gear_box.h"
#include "ui_shift_alert.h"
#include "center-colors.h"

LV_FONT_DECLARE(aerospace_188);
LV_FONT_DECLARE(aerospace_87);
LV_FONT_DECLARE(racehead_14);
LV_FONT_DECLARE(racehead_22);

#define CX 400
#define CY 400
#define BOX_W 244
#define BOX_H 244
#define RPM_DIG_Y  (CY - 167)
#define RPM_LBL_Y  (CY - 133)   /* RPM unit label, just above the gear-box */

static lv_obj_t *s_box;
static lv_obj_t *s_gear_lbl;
static lv_obj_t *s_gear_unit;
static lv_obj_t *s_rpm_lbl;
static lv_obj_t *s_rpm_unit;

void ui_gear_box_raise(void)
{
    if (s_box) {
        lv_obj_move_foreground(s_box);
    }
    if (s_gear_lbl) {
        lv_obj_move_foreground(s_gear_lbl);
    }
    if (s_gear_unit) {
        lv_obj_move_foreground(s_gear_unit);
    }
    if (s_rpm_lbl) {
        lv_obj_move_foreground(s_rpm_lbl);
    }
    if (s_rpm_unit) {
        lv_obj_move_foreground(s_rpm_unit);
    }
}

void ui_gear_box_create(lv_obj_t *parent)
{
    /* Gear box */
    s_box = lv_obj_create(parent);
    lv_obj_set_size(s_box, BOX_W, BOX_H);
    lv_obj_align(s_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_box, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(s_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_box, 1, 0);
    lv_obj_set_style_border_color(s_box, COLOR_WHITE, 0);
    lv_obj_set_style_border_opa(s_box, LV_OPA_50, 0);
    lv_obj_set_style_radius(s_box, 0, 0);
    lv_obj_clear_flag(s_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Gear glyph — Aerospace 188 px */
    s_gear_lbl = lv_label_create(s_box);
    lv_label_set_text(s_gear_lbl, "N");
    lv_obj_set_style_text_color(s_gear_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(s_gear_lbl, &aerospace_188, 0);
    lv_obj_align(s_gear_lbl, LV_ALIGN_CENTER, 0, 4);

    /* "GEAR" caption inside the box, 20 px below the gear glyph — RaceHead 22 px, white */
    s_gear_unit = lv_label_create(s_box);
    lv_label_set_text(s_gear_unit, "GEAR");
    lv_obj_set_style_text_color(s_gear_unit, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(s_gear_unit, &racehead_22, 0);
    lv_obj_align(s_gear_unit, LV_ALIGN_CENTER, 0, 90);

    /* RPM digital readout — Aerospace 87 px */
    s_rpm_lbl = lv_label_create(parent);
    lv_label_set_text(s_rpm_lbl, "0");
    lv_obj_set_style_text_color(s_rpm_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(s_rpm_lbl, &aerospace_87, 0);
    lv_obj_set_style_text_align(s_rpm_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_rpm_lbl, LV_ALIGN_TOP_MID, 0, RPM_DIG_Y - CY + 400 - 43);

    /* RPM unit label — RaceHead 22 px (enlarged) */
    s_rpm_unit = lv_label_create(parent);
    lv_label_set_text(s_rpm_unit, "RPM");
    lv_obj_set_style_text_color(s_rpm_unit, COLOR_WHITE, 0);
    lv_obj_set_style_text_opa(s_rpm_unit, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_rpm_unit, &racehead_22, 0);
    lv_obj_align(s_rpm_unit, LV_ALIGN_TOP_MID, 0, RPM_LBL_Y - CY + 400 - 7);
}

void ui_gear_box_update(const dash_data_t *d)
{
    /* Gear glyph */
    char gear_str[4];
    if (d->gear == 0)       lv_snprintf(gear_str, sizeof(gear_str), "N");
    else if (d->gear == -1) lv_snprintf(gear_str, sizeof(gear_str), "R");
    else                    lv_snprintf(gear_str, sizeof(gear_str), "%d", (int)d->gear);
    lv_label_set_text(s_gear_lbl, gear_str);

    /* Box tint — transparent below 3k, then color/opacity ramp (no strobe). */
    float ramp = 0.0f;
    lv_color_t tint_col = COLOR_BG_PRIMARY;
    if (d->rpm >= 3000.0f) {
        ramp = LV_MIN((d->rpm - 3000.0f) / 4000.0f, 1.0f) * 0.54f + 0.08f;
        if      (d->rpm < 4001.0f) tint_col = COLOR_GREEN;
        else if (d->rpm < 6401.0f) tint_col = COLOR_GOLD;
        else                       tint_col = COLOR_RED_HOT;
    }

    if (ui_shift_alert_strobe_active()) {
        ramp = ui_shift_alert_flash_on() ? LV_MIN(ramp + 0.12f, 0.74f) : ramp * 0.28f;
    }

    lv_obj_set_style_bg_color(s_box, tint_col, 0);
    lv_obj_set_style_bg_opa(s_box,
                            d->rpm < 3000.0f ? LV_OPA_TRANSP : (lv_opa_t)(ramp * 255),
                            0);

    /* RPM digital color + flash */
    int seg_idx = (int)(d->rpm / 250.0f);
    if (seg_idx > 31) seg_idx = 31;
    lv_color_t rpm_col = rpm_seg_color(seg_idx, true);
    lv_label_set_text_fmt(s_rpm_lbl, "%d", (int)d->rpm);
    lv_obj_set_style_text_color(s_rpm_lbl, rpm_col, 0);
}
