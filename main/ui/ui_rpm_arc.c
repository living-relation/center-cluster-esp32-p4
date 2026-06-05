/**
 * ui_rpm_arc.c — 32-segment 270° RPM arc, center cluster.
 *
 * Geometry (native 800×800):
 *   center=(400,400)  RO=369  RI=309
 *   start=135°  sweep=270°  32 segments  gap=1.5°
 *
 * Each segment is a 4-corner trapezoid (2 triangles). No arc-subdivision steps —
 * avoids horizontal banding from quad-strip tessellation. Tiny draw-only angular
 * bleed closes inter-segment seams without changing tick label layout.
 */
#include "ui_rpm_arc.h"
#include "center-colors.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <string.h>

LV_FONT_DECLARE(racehead_24);

#define RPM_MAX     8000.0f
#define N_SEGS      32
#define CX          400
#define CY          400
#define RO          369
#define RI          309
#define START_DEG   135.0f
#define TOTAL_DEG   270.0f
#define GAP_DEG         1.5f   /* layout + tick labels */
#define GAP_BLEED_DEG   0.35f  /* draw-only: slight overlap into nominal gaps */

static lv_obj_t   *s_canvas = NULL;
static lv_color_t *s_buf    = NULL;
static float       s_seg_fill_prev[N_SEGS];

static lv_point_precise_t s_wedge_pts[N_SEGS][4];

static float deg_to_rad(float d) { return d * (float)M_PI / 180.0f; }

static void wedge_corner(float deg, float radius, lv_point_precise_t *out)
{
    float r = deg_to_rad(deg - 90.0f);
    out->x = (lv_coord_t)((float)CX + radius * cosf(r));
    out->y = (lv_coord_t)((float)CY + radius * sinf(r));
}

static void precompute_wedges(void)
{
    float seg_span = (TOTAL_DEG - GAP_DEG * N_SEGS) / N_SEGS;
    float bleed = GAP_BLEED_DEG * 0.5f;

    for (int i = 0; i < N_SEGS; i++) {
        float layout_a0 = START_DEG + (float)i * (seg_span + GAP_DEG);
        float layout_a1 = layout_a0 + seg_span;
        float a0 = layout_a0 - bleed;
        float a1 = layout_a1 + bleed;

        wedge_corner(a0, (float)RO, &s_wedge_pts[i][0]);
        wedge_corner(a1, (float)RO, &s_wedge_pts[i][1]);
        wedge_corner(a1, (float)RI, &s_wedge_pts[i][2]);
        wedge_corner(a0, (float)RI, &s_wedge_pts[i][3]);
    }
}

static lv_color_t seg_color(int i, bool lit)
{
    return rpm_seg_color(i, lit);
}

static void draw_trapezoid(lv_layer_t *layer, const lv_point_precise_t *pts,
                           lv_color_t color, lv_opa_t opa)
{
    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa   = opa;
    dsc.p[0] = pts[0];
    dsc.p[1] = pts[1];
    dsc.p[2] = pts[2];
    lv_draw_triangle(layer, &dsc);
    dsc.p[0] = pts[0];
    dsc.p[1] = pts[2];
    dsc.p[2] = pts[3];
    lv_draw_triangle(layer, &dsc);
}

static void draw_segment(lv_layer_t *layer, int i, float intensity)
{
    const lv_point_precise_t *pts = s_wedge_pts[i];

    lv_color_t c_off = seg_color(i, false);
    lv_color_t c_on  = seg_color(i, true);

    draw_trapezoid(layer, pts, c_off, LV_OPA_COVER);

    if (intensity > 0.0f) {
        draw_trapezoid(layer, pts, c_on, (lv_opa_t)(intensity * 255.0f));
    }

    if (intensity > 0.0f && i >= 27) {
        draw_trapezoid(layer, pts, COLOR_RED_HOT,
                       (lv_opa_t)(intensity * 0.50f * 255.0f));
    }
}

void ui_rpm_arc_create(lv_obj_t *parent)
{
    precompute_wedges();

    s_buf = (lv_color_t *)heap_caps_malloc(
        (size_t)800 * 800 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!s_buf) return;

    s_canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_canvas, s_buf, 800, 800, LV_COLOR_FORMAT_NATIVE);
    lv_obj_center(s_canvas);
    lv_canvas_fill_bg(s_canvas, COLOR_BG_PRIMARY, LV_OPA_COVER);

    for (int i = 0; i < N_SEGS; i++) {
        s_seg_fill_prev[i] = -1.0f;
    }

    for (int k = 1; k <= 8; k++) {
        float deg = START_DEG + (float)(k * 4) * (TOTAL_DEG / (float)N_SEGS);
        float r   = deg_to_rad(deg - 90.0f);
        int dx = (int)lroundf(291.0f * cosf(r));
        int dy = (int)lroundf(291.0f * sinf(r));
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text_fmt(lbl, "%d", k);
        lv_obj_set_style_text_color(lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(lbl, LV_OPA_40, 0);
        lv_obj_set_style_text_font(lbl, &racehead_24, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, dx, dy);
    }
}

void ui_rpm_arc_update(const dash_data_t *d)
{
    if (!s_canvas) return;

    float rpm = d->rpm;
    if (rpm < 0.0f) rpm = 0.0f;
    if (rpm > RPM_MAX) rpm = RPM_MAX;
    const float lit_f = rpm / (RPM_MAX / N_SEGS);

    lv_layer_t layer;
    lv_canvas_init_layer(s_canvas, &layer);
    for (int i = 0; i < N_SEGS; i++) {
        float seg_fill = lit_f - (float)i;
        if (seg_fill < 0.0f) seg_fill = 0.0f;
        if (seg_fill > 1.0f) seg_fill = 1.0f;

        if (fabsf(seg_fill - s_seg_fill_prev[i]) >= (1.0f / 255.0f)) {
            draw_segment(&layer, i, seg_fill);
            s_seg_fill_prev[i] = seg_fill;
        }
    }
    lv_canvas_finish_layer(s_canvas, &layer);
}
