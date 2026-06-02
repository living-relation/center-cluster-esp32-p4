/**
 * center-colors.h — TrackCluster design tokens for LVGL
 *
 * Mirrors `colors_and_type.css`. Self-contained per-cluster copy.
 */

#ifndef CENTER_COLORS_H
#define CENTER_COLORS_H

#include "lvgl.h"

/* ── Backgrounds ───────────────────────────────────────────────────────── */
#define COLOR_BG_PRIMARY      lv_color_hex(0x000000)
#define COLOR_BG_SECONDARY    lv_color_hex(0x0A0A0A)
#define COLOR_BG_CARD         lv_color_hex(0x111111)

/* ── Channel / state colors ────────────────────────────────────────────── */
#define COLOR_GREEN           lv_color_hex(0x28FF00)   // normal, oil press normal
#define COLOR_GREEN_DIM       lv_color_hex(0x14801A)
#define COLOR_YELLOW          lv_color_hex(0xFFD400)   // RPM mid, IAT warm
#define COLOR_GOLD            lv_color_hex(0xFFD700)   // gear box yellow zone
#define COLOR_ORANGE          lv_color_hex(0xFF9800)   // caution, MPH default
#define COLOR_DEEP_ORANGE     lv_color_hex(0xFF5722)
#define COLOR_RED             lv_color_hex(0xF44336)
#define COLOR_RED_HOT         lv_color_hex(0xFF1744)   // redline / alarm
#define COLOR_RED_LED         lv_color_hex(0xFF0E1A)   // shift LED red
#define COLOR_CYAN            lv_color_hex(0x00BCD4)   // mini-arc text, fuel normal
#define COLOR_PURPLE          lv_color_hex(0x9C27B0)
#define COLOR_PINK            lv_color_hex(0xE91E63)

/* ── Neutrals / text ───────────────────────────────────────────────────── */
#define COLOR_WHITE           lv_color_white()
#define COLOR_GREY            lv_color_hex(0x888888)
#define COLOR_GREY_DARK       lv_color_hex(0x333333)
#define COLOR_INACTIVE        lv_color_hex(0x1A1A1A)   // unlit segment

/* ── Toyota brand ──────────────────────────────────────────────────────── */
#define COLOR_TOYOTA_RED      lv_color_hex(0xEB0A1E)
#define COLOR_TOYOTA_SILVER   lv_color_hex(0xB0B0B0)

/* ── Opacity helpers ────────────────────────────────────────────────────── */
#define OPA_LABEL_NORMAL      LV_OPA_40
#define OPA_LABEL_DIM         LV_OPA_30
#define OPA_LABEL_FAINT       LV_OPA_20

/* ── Shift-LED color by index 0..9 with all-red hysteresis ──────────────── *
 * Caller updates `all_red` based on RPM crossings (>=7000 latch, <6500 release)
 * BEFORE calling this function. See `center-07-state-machines.md`.
 */
static inline lv_color_t shift_led_color(int idx, bool all_red) {
    if (all_red) return COLOR_RED_LED;
    if (idx < 5) return COLOR_GREEN;
    if (idx < 8) return COLOR_YELLOW;
    return COLOR_RED_LED;
}

/* ── RPM segment color by index 0..31 ──────────────────────────────────── */
static inline lv_color_t rpm_seg_color(int idx, bool lit) {
    if (!lit)        return COLOR_INACTIVE;
    if (idx < 24)    return COLOR_WHITE;                // 0..6000 rpm
    if (idx < 27) {                                     // 6000..6750 gradient
        uint8_t t = (idx - 24) * 85;
        uint8_t r = 255;
        uint8_t g = 255 - (uint8_t)((255 - 23) * t / 255);
        uint8_t b = 255 - (uint8_t)((255 - 68) * t / 255);
        return lv_color_make(r, g, b);
    }
    return COLOR_RED_HOT;                               // 6750..8000
}

/* ── Lambda color ramp ─────────────────────────────────────────────────── *
 * Widened for 93-octane pump gas AND E85. Both fuels target λ ≈ 0.78–0.85
 * under boost (rich for cooling/knock margin). Stoich cruise is 1.00.
 *   < 0.70           red    (catastrophic rich / flooding)
 *   0.70..0.749      orange (very rich edge)
 *   0.75..1.019      green  (covers boosted WOT and stoich)
 *   1.02..1.049      orange (lean transition)
 *   >= 1.05          red    (lean alarm)
 */
static inline lv_color_t lambda_color(float l) {
    if (l < 0.70f)  return COLOR_RED_HOT;
    if (l < 0.75f)  return COLOR_ORANGE;
    if (l < 1.02f)  return COLOR_GREEN;
    if (l < 1.05f)  return COLOR_ORANGE;
    return COLOR_RED_HOT;
}

/* ── Boost color ramp ──────────────────────────────────────────────────── */
static inline lv_color_t boost_color(float psi) {
    if (psi < 14.0f) return COLOR_CYAN;
    if (psi < 20.0f) return COLOR_ORANGE;
    return COLOR_RED_HOT;
}

/* ── Coolant temp ramp ─────────────────────────────────────────────────── */
static inline lv_color_t ect_color(float f) {
    if (f < 150.0f) return COLOR_CYAN;
    if (f < 200.0f) return COLOR_WHITE;
    if (f < 220.0f) return COLOR_ORANGE;
    return COLOR_RED_HOT;
}

/* ── IAT ramp (bar visible range 30..200 °F) ────────────────────────────── */
static inline lv_color_t iat_color(float f) {
    if (f > 150.0f) return COLOR_RED_HOT;
    if (f > 130.0f) return COLOR_ORANGE;
    if (f > 100.0f) return COLOR_YELLOW;
    return COLOR_GREEN;
}

/* ── Ignition advance ramp ─────────────────────────────────────────────── */
static inline lv_color_t ign_color(float deg) {
    if (deg < 8.0f  || deg > 38.0f) return COLOR_RED_HOT;
    if (deg < 12.0f || deg > 32.0f) return COLOR_ORANGE;
    return COLOR_GREEN;
}

/* ── Fuel-level ramp ───────────────────────────────────────────────────── */
static inline lv_color_t fuel_color(float pct) {
    if (pct < 15.0f) return COLOR_RED_HOT;
    if (pct < 30.0f) return COLOR_ORANGE;
    return COLOR_CYAN;
}

/* ── Fuel-pressure ramp (display range 20..160 PSI) ─────────────────────── */
static inline lv_color_t fuel_press_color(float psi) {
    if (psi < 30.0f || psi > 125.0f) return COLOR_RED_HOT;
    return COLOR_CYAN;
}

#endif /* CENTER_COLORS_H */
