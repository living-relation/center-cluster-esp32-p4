/**
 * ui.c — top-level LVGL UI coordinator, center cluster.
 *
 * Wires together boot splash and live cluster screen. All geometry is in
 * native 800×800 pixels (see center geometry contract in the consolidated SPEC).
 *
 * Visual reference: LVGL Simulator.html (center cluster) — kept in sync with
 * this code in both directions. When the UI changes, the simulator changes too.
 */

#include "ui.h"
#include "ui_rpm_arc.h"
#include "ui_shift_leds.h"
#include "ui_gear_box.h"
#include "ui_odometer.h"
#include "ui_boot.h"
#include "dash_data.h"
#include "center-colors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_log.h"

static const char *TAG = "ui";
extern portMUX_TYPE g_dash_mux;

#define UI_PAINT_TICK_MS 8   /* 125 Hz UI update cadence, no display-side lag */

static lv_obj_t *s_screen  = NULL;
static bool      s_live    = false;   /* false during boot splash */

/* ── Paint-tick snapshot ─────────────────────────────────────────────── */
void ui_paint_tick(lv_timer_t *t)
{
    if (!s_live) return;

    dash_data_t snap;
    portENTER_CRITICAL(&g_dash_mux);
    snap = *(const dash_data_t *)&g_dash;
    portEXIT_CRITICAL(&g_dash_mux);

    ui_rpm_arc_update(&snap);
    ui_shift_leds_update(&snap);
    ui_gear_box_update(&snap);
    ui_odometer_update(&snap);
}

/* ── Boot complete callback (called by ui_boot.c after splash) ───────── */
void ui_on_boot_complete(void)
{
    ESP_LOGI(TAG, "Boot splash done — going live");
    s_live = true;
    lv_timer_create(ui_paint_tick, UI_PAINT_TICK_MS, NULL);
}

/* ── Init ────────────────────────────────────────────────────────────── */
void ui_init(lv_disp_t *disp)
{
    /* Black base screen */
    s_screen = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(s_screen, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);

    /* Build widget tree (hidden until boot splash finishes) */
    ui_rpm_arc_create(s_screen);
    ui_shift_leds_create(s_screen);
    ui_gear_box_create(s_screen);
    ui_odometer_create(s_screen);

    /* Boot splash — runs on top, calls ui_on_boot_complete() when done */
    ui_boot_start(s_screen, ui_on_boot_complete);
}
