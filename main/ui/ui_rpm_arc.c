/**
 * ui_rpm_arc.c — 32-segment 270° RPM arc, center cluster.
 *
 * Uses custom lv_canvas + draw_wedge polygon per the reference implementation
 * in center-06-lvgl-widgets.md §"Custom-draw segmented arc".
 *
 * Geometry (native 800×800):
 *   center=(400,400)  RO=369  RI=309
 *   start=135°  sweep=270°  32 segments  gap=1.5°
 *   each segment span = (270 − 1.5×32) / 32 = 6.9375°
 *
 * Color per segment index i (0=bottom-left, 31=bottom-right):
 *   i  0–23: WHITE (#FFFFFF) lit, INACTIVE (#1A1A1A) unlit
 *   i 24–26: WHITE→RED gradient lit (lerp by (i-24)/3), INACTIVE unlit
 *   i 27–31: RED_HOT (#FF1744) lit + 3px red glow, INACTIVE unlit
 *
 * Wedge geometry is constant — precomputed once at create time.
 * On each tick only the fill color per segment is updated.
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
#define GAP_DEG     1.5f
#define WEDGE_STEPS 8   /* polygon approx steps per arc edge */

static lv_obj_t   *s_canvas = NULL;
static lv_color_t *s_buf    = NULL;

/* Pre-computed polygon point arrays — computed once at create time. */
#define PTS_PER_WEDGE ((WEDGE_STEPS + 1) * 2)
static lv_point_precise_t s_wedge_pts[N_SEGS][PTS_PER_WEDGE];
static int        s_wedge_n  [N_SEGS];

static float deg_to_rad(float d) { return d * (float)M_PI / 180.0f; }

static void precompute_wedges(void)
{
    float seg_span = (TOTAL_DEG - GAP_DEG * N_SEGS) / N_SEGS;

    for (int i = 0; i < N_SEGS; i++) {
        float a0 = START_DEG + i * (seg_span + GAP_DEG);
        float a1 = a0 + seg_span;
        lv_point_precise_t *pts = s_wedge_pts[i];
        int n = 0;

        /* Outer arc points (a0 → a1) */
        for (int s = 0; s <= WEDGE_STEPS; s++) {
            float a = a0 + (a1 - a0) * (float)s / WEDGE_STEPS;
            float r = deg_to_rad(a - 90.0f);
            pts[n].x = CX + RO * cosf(r);
            pts[n].y = CY + RO * sinf(r);
            n++;
        }
        /* Inner arc points (a1 → a0, reversed) */
        for (int s = WEDGE_STEPS; s >= 0; s--) {
            float a = a0 + (a1 - a0) * (float)s / WEDGE_STEPS;
            float r = deg_to_rad(a - 90.0f);
            pts[n].x = CX + RI * cosf(r);
            pts[n].y = CY + RI * sinf(r);
            n++;
        }
        s_wedge_n[i] = n;
    }
}

static lv_color_t seg_color(int i, bool lit)
{
    if (!lit) return COLOR_INACTIVE;
    if (i < 24) return COLOR_WHITE;
    if (i < 27) {
        /* Gradient: WHITE (#FFFFFF) → RED_HOT (#FF1744), lerp by (i-24)/3 */
        float t = (float)(i - 24) / 3.0f;
        uint8_t r = 255;
        uint8_t g = (uint8_t)(255 * (1.0f - t));
        uint8_t b = (uint8_t)((255 * (1.0f - t)) * (68.0f / 255.0f));
        return lv_color_make(r, g, b);
    }
    return COLOR_RED_HOT;
}

static void draw_segment(lv_layer_t *layer, int i, bool lit)
{
    const lv_point_precise_t *pts = s_wedge_pts[i];
    int n = s_wedge_n[i];

    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.bg_opa   = LV_OPA_COVER;
    dsc.bg_color = seg_color(i, lit);

    /* Fan triangulation: pts[0] is the hub */
    for (int k = 1; k < n - 1; k++) {
        dsc.p[0] = pts[0];
        dsc.p[1] = pts[k];
        dsc.p[2] = pts[k + 1];
        lv_draw_triangle(layer, &dsc);
    }

    /* Glow overlay for red-zone segments when lit */
    if (lit && i >= 27) {
        lv_draw_triangle_dsc_t glow;
        lv_draw_triangle_dsc_init(&glow);
        glow.bg_color = COLOR_RED_HOT;
        glow.bg_opa   = (lv_opa_t)(0.50f * 255);
        for (int k = 1; k < n - 1; k++) {
            glow.p[0] = pts[0];
            glow.p[1] = pts[k];
            glow.p[2] = pts[k + 1];
            lv_draw_triangle(layer, &glow);
        }
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

    /* RPM scale labels 1–8 at radius 291 (just inside RI, moved 10 px inward),
     * RaceHead 24 px, white @ 40 %%. Placed at each 1000-RPM segment boundary. */
    for (int k = 1; k <= 8; k++) {
        float deg = START_DEG + (float)(k * 4) * (TOTAL_DEG / N_SEGS); /* = 135 + k*33.75 */
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

    int lit = (int)(d->rpm / (RPM_MAX / N_SEGS));
    if (lit > N_SEGS) lit = N_SEGS;
    if (lit < 0)      lit = 0;

    lv_canvas_fill_bg(s_canvas, COLOR_BG_PRIMARY, LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(s_canvas, &layer);
    for (int i = 0; i < N_SEGS; i++) {
        draw_segment(&layer, i, i < lit);
    }
    lv_canvas_finish_layer(s_canvas, &layer);
}
