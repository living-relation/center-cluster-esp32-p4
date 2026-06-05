/**
 * ui_boot.c — Toyota boot splash, center cluster (matches right cluster).
 *
 * Timeline:
 *   black hold → logo fades in over black → hold 5 s → overlay fades out → live UI
 *
 * Fade-in animates logo opacity (overlay stays opaque black). Fade-out animates the
 * full overlay so the gauges underneath appear gradually.
 */
#include "ui_boot.h"
#include "bsp.h"
#include "lvgl.h"

LV_IMAGE_DECLARE(toyota_logo);

#define BOOT_BLACK_HOLD_MS  150
#define BOOT_FADE_IN_MS     1200
#define BOOT_HOLD_MS        5000
#define BOOT_FADE_OUT_MS    1200

static ui_boot_done_cb_t s_cb       = NULL;
static lv_obj_t         *s_overlay = NULL;

static void boot_img_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_image_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void boot_overlay_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void boot_teardown(void)
{
    if (s_overlay) {
        lv_obj_del(s_overlay);
        s_overlay = NULL;
    }
    if (s_cb) {
        ui_boot_done_cb_t cb = s_cb;
        s_cb = NULL;
        cb();
    }
}

static void boot_fade_out_ready_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    boot_teardown();
}

static uint32_t fit_scale_q8(uint32_t img_w, uint32_t img_h)
{
    if (img_w == 0 || img_h == 0) {
        return 256;
    }

    uint32_t sx = ((uint32_t)BSP_LCD_H_RES * 256U) / img_w;
    uint32_t sy = ((uint32_t)BSP_LCD_V_RES * 256U) / img_h;
    uint32_t s  = (sx < sy) ? sx : sy;

    if (s > 256U) {
        s = 256U;
    }
    if (s < 1U) {
        s = 1U;
    }
    return s;
}

void ui_boot_start(lv_obj_t *parent, ui_boot_done_cb_t on_done)
{
    s_cb = on_done;

    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_center(s_overlay);
    lv_obj_set_style_bg_color(s_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);
    lv_obj_set_style_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(s_overlay);

    uint32_t fit_q8 = fit_scale_q8(toyota_logo.header.w, toyota_logo.header.h);

    lv_obj_t *img = lv_image_create(s_overlay);
    lv_image_set_src(img, &toyota_logo);
    lv_obj_set_size(img, (lv_coord_t)toyota_logo.header.w, (lv_coord_t)toyota_logo.header.h);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_image_set_scale_x(img, fit_q8);
    lv_image_set_scale_y(img, fit_q8);
    lv_image_set_antialias(img, true);
    lv_obj_set_style_image_opa(img, LV_OPA_TRANSP, 0);

    lv_anim_t fade_in;
    lv_anim_init(&fade_in);
    lv_anim_set_var(&fade_in, img);
    lv_anim_set_exec_cb(&fade_in, boot_img_opa_cb);
    lv_anim_set_path_cb(&fade_in, lv_anim_path_ease_in_out);
    lv_anim_set_time(&fade_in, BOOT_FADE_IN_MS);
    lv_anim_set_delay(&fade_in, BOOT_BLACK_HOLD_MS);
    lv_anim_set_values(&fade_in, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_start(&fade_in);

    lv_anim_t fade_out;
    lv_anim_init(&fade_out);
    lv_anim_set_var(&fade_out, s_overlay);
    lv_anim_set_exec_cb(&fade_out, boot_overlay_opa_cb);
    lv_anim_set_path_cb(&fade_out, lv_anim_path_ease_in_out);
    lv_anim_set_time(&fade_out, BOOT_FADE_OUT_MS);
    lv_anim_set_delay(&fade_out, BOOT_BLACK_HOLD_MS + BOOT_FADE_IN_MS + BOOT_HOLD_MS);
    lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_completed_cb(&fade_out, boot_fade_out_ready_cb);
    lv_anim_start(&fade_out);
}
