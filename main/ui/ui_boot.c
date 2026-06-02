/**
 * ui_boot.c — animated Toyota boot splash, center cluster.
 *
 * Timeline (1800 ms total) per center-07-state-machines.md §7:
 *   0–100 ms    black hold
 *   100–700 ms  mark fades + scales in (opacity 0→1, scale 60%→100%, ease-out)
 *   500–1000 ms wordmark slides up + fades in (y +30→0, opacity 0→1, ease-out)
 *   1000–1500 ms hold
 *   1500–1800 ms both fade out together
 *   1800 ms     on_done() fires
 *
 * Assets compiled from main/ui/assets/toyota_mark.c + toyota_wordmark.c
 * (auto-generated LVGL ARGB8888 C arrays from assets/toyota-mark.png etc.)
 */

#include "ui_boot.h"
#include "center-colors.h"

LV_IMAGE_DECLARE(toyota_mark);
LV_IMAGE_DECLARE(toyota_wordmark);

static ui_boot_done_cb_t s_cb      = NULL;
static lv_obj_t         *s_overlay = NULL;
static lv_obj_t         *s_mark    = NULL;
static lv_obj_t         *s_word    = NULL;

/* ── lv_anim exec callbacks ─────────────────────────────────────────── */
static void set_img_opa  (void *obj, int32_t v) { lv_obj_set_style_image_opa((lv_obj_t *)obj, (lv_opa_t)v, 0); }
static void set_img_scale(void *obj, int32_t v) { lv_image_set_scale((lv_obj_t *)obj, (uint16_t)v); }
static void set_y        (void *obj, int32_t v) { lv_obj_set_y((lv_obj_t *)obj, (lv_coord_t)v); }

static void splash_done_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    if (s_mark)    { lv_obj_del(s_mark);    s_mark    = NULL; }
    if (s_word)    { lv_obj_del(s_word);    s_word    = NULL; }
    if (s_overlay) { lv_obj_del(s_overlay); s_overlay = NULL; }
    if (s_cb) s_cb();
}

static void splash_play(void)
{
    lv_anim_t a;

    /* Mark: opacity + scale, 100→700 ms */
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_mark);
    lv_anim_set_time(&a, 600);
    lv_anim_set_delay(&a, 100);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    lv_anim_set_exec_cb(&a, set_img_opa);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_img_scale);
    lv_anim_set_values(&a, 154, 256);   /* 60% → 100% */
    lv_anim_start(&a);

    /* Wordmark: opacity + y, 500→1000 ms */
    int word_final_y = 400 + 130;
    lv_anim_set_var(&a, s_word);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 400);

    lv_anim_set_exec_cb(&a, set_img_opa);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_y);
    lv_anim_set_values(&a, word_final_y + 30, word_final_y);
    lv_anim_start(&a);

    /* Fade-out both, 1500→1800 ms */
    lv_anim_set_time(&a, 300);
    lv_anim_set_delay(&a, 1500);
    lv_anim_set_exec_cb(&a, set_img_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);

    lv_anim_set_var(&a, s_mark);
    lv_anim_start(&a);
    lv_anim_set_var(&a, s_word);
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

    /* Mark — centered at (cx, cy−50) = (400, 350), starts at 60% scale */
    s_mark = lv_image_create(s_overlay);
    lv_image_set_src(s_mark, &toyota_mark);
    lv_obj_align(s_mark, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_image_opa(s_mark, LV_OPA_TRANSP, 0);
    lv_image_set_scale(s_mark, 154);

    /* Wordmark — final center at (cx, cy+130) = (400, 530), starts 30px lower */
    s_word = lv_image_create(s_overlay);
    lv_image_set_src(s_word, &toyota_wordmark);
    lv_obj_align(s_word, LV_ALIGN_CENTER, 0, 130);
    lv_obj_set_style_image_opa(s_word, LV_OPA_TRANSP, 0);

    splash_play();
}
