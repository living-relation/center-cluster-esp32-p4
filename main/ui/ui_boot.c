/**
 * ui_boot.c — animated Toyota boot splash, center cluster.
 *
 * Timeline (1800 ms total) per center-07-state-machines.md §7:
 *   0–100 ms    black hold
 *   100–700 ms  logo fades + scales in (opacity 0→1, scale 60%→100%, ease-out)
 *   1000–1500 ms hold
 *   1500–1800 ms logo fades out
 *   1800 ms     on_done() fires
 *
 * Asset compiled from main/ui/assets/toyota-logo.c
 * (auto-generated LVGL ARGB8888 C array from the original PNG)
 */

#include "ui_boot.h"
#include "bsp.h"
#include "center-colors.h"

LV_IMAGE_DECLARE(toyota_logo);

static ui_boot_done_cb_t s_cb      = NULL;
static lv_obj_t         *s_overlay = NULL;
static lv_obj_t         *s_mark    = NULL;
static int32_t           s_logo_scale_start_q8 = 154;
static int32_t           s_logo_scale_final_q8 = 256;

/* ── lv_anim exec callbacks ─────────────────────────────────────────── */
static void set_img_opa  (void *obj, int32_t v) { lv_obj_set_style_image_opa((lv_obj_t *)obj, (lv_opa_t)v, 0); }
static void set_img_scale_uniform(void *obj, int32_t v)
{
    lv_obj_t *o = (lv_obj_t *)obj;
    lv_image_set_scale_x(o, (uint32_t)v);
    lv_image_set_scale_y(o, (uint32_t)v);
}

static uint32_t fit_scale_q8(uint32_t img_w, uint32_t img_h)
{
    if (img_w == 0 || img_h == 0) {
        return 256;
    }

    uint32_t sx = ((uint32_t)BSP_LCD_H_RES * 256U) / img_w;
    uint32_t sy = ((uint32_t)BSP_LCD_V_RES * 256U) / img_h;
    uint32_t s  = (sx < sy) ? sx : sy;

    if (s > 256U) s = 256U;
    if (s < 1U) s = 1U;
    return s;
}

static void splash_done_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    if (s_mark)    { lv_obj_del(s_mark);    s_mark    = NULL; }
    if (s_overlay) { lv_obj_del(s_overlay); s_overlay = NULL; }
    if (s_cb) s_cb();
}

static void splash_play(void)
{
    lv_anim_t a;

    /* Logo: opacity + scale, 100→700 ms */
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_mark);
    lv_anim_set_time(&a, 600);
    lv_anim_set_delay(&a, 100);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    lv_anim_set_exec_cb(&a, set_img_opa);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_img_scale_uniform);
    lv_anim_set_values(&a, s_logo_scale_start_q8, s_logo_scale_final_q8);
    lv_anim_start(&a);

    /* Fade-out logo, 1500→1800 ms */
    lv_anim_set_time(&a, 300);
    lv_anim_set_delay(&a, 1500);
    lv_anim_set_exec_cb(&a, set_img_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);

    lv_anim_set_var(&a, s_mark);
    lv_anim_start(&a);

    lv_timer_t *t = lv_timer_create(splash_done_cb, 1800, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void ui_boot_start(lv_obj_t *parent, ui_boot_done_cb_t on_done)
{
    s_cb = on_done;

    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, 800, 800);
    lv_obj_center(s_overlay);
    lv_obj_set_style_bg_color(s_overlay, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(s_overlay);

    uint32_t fit_q8 = fit_scale_q8(toyota_logo.header.w, toyota_logo.header.h);
    s_logo_scale_final_q8 = (int32_t)fit_q8;
    s_logo_scale_start_q8 = (int32_t)((fit_q8 * 154U) / 256U);
    if (s_logo_scale_start_q8 < 1) {
        s_logo_scale_start_q8 = 1;
    }

    /* Single logo centered, always uniform aspect-fit to display bounds. */
    s_mark = lv_image_create(s_overlay);
    lv_image_set_src(s_mark, &toyota_logo);
    lv_obj_set_size(s_mark, (lv_coord_t)toyota_logo.header.w, (lv_coord_t)toyota_logo.header.h);
    lv_image_set_inner_align(s_mark, LV_IMAGE_ALIGN_CENTER);
    lv_obj_align(s_mark, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_image_opa(s_mark, LV_OPA_TRANSP, 0);
    lv_image_set_scale_x(s_mark, (uint32_t)s_logo_scale_start_q8);
    lv_image_set_scale_y(s_mark, (uint32_t)s_logo_scale_start_q8);

    splash_play();
}
