#include "ui_theme.h"

static const ui_theme_palette_t k_theme_palettes[UI_THEME_COUNT] = {
    [UI_THEME_SPORT] = {
        .screen_bg_color = LV_COLOR_MAKE(0x05, 0x08, 0x16),
        .rpm_color_low = LV_COLOR_MAKE(0x39, 0xFF, 0x14),
        .rpm_color_mid = LV_COLOR_MAKE(0xFF, 0xE3, 0x47),
        .rpm_color_high = LV_COLOR_MAKE(0xFF, 0x36, 0x5E),
        .mph_value_color = LV_COLOR_MAKE(0x00, 0xE5, 0xFF),
        .primary_label_color = LV_COLOR_MAKE(0xEA, 0xF2, 0xFF),
        .gear_value_color = LV_COLOR_MAKE(0xFF, 0x4F, 0xD8),
        .odometer_color = LV_COLOR_MAKE(0x44, 0xFF, 0xAA),
    },
    [UI_THEME_OEM] = {
        .screen_bg_color = LV_COLOR_MAKE(0x11, 0x14, 0x18),
        .rpm_color_low = LV_COLOR_MAKE(0xC7, 0xD3, 0xE0),
        .rpm_color_mid = LV_COLOR_MAKE(0x9E, 0xB6, 0xCF),
        .rpm_color_high = LV_COLOR_MAKE(0x7F, 0xAE, 0xFF),
        .mph_value_color = LV_COLOR_MAKE(0xF7, 0xF9, 0xFC),
        .primary_label_color = LV_COLOR_MAKE(0xA7, 0xB0, 0xBA),
        .gear_value_color = LV_COLOR_MAKE(0x7F, 0xAE, 0xFF),
        .odometer_color = LV_COLOR_MAKE(0xD0, 0xD7, 0xDF),
    },
    [UI_THEME_STEALTH] = {
        .screen_bg_color = LV_COLOR_MAKE(0x03, 0x03, 0x03),
        .rpm_color_low = LV_COLOR_MAKE(0xFF, 0x9F, 0x1A),
        .rpm_color_mid = LV_COLOR_MAKE(0xFF, 0xB3, 0x47),
        .rpm_color_high = LV_COLOR_MAKE(0xFF, 0x5E, 0x00),
        .mph_value_color = LV_COLOR_MAKE(0xED, 0xED, 0xED),
        .primary_label_color = LV_COLOR_MAKE(0x7A, 0x7A, 0x7A),
        .gear_value_color = LV_COLOR_MAKE(0xFF, 0xB3, 0x47),
        .odometer_color = LV_COLOR_MAKE(0x9A, 0xA0, 0xA6),
    },
};

static ui_theme_t g_active_theme = UI_THEME_DEFAULT;

static ui_theme_t ui_theme_normalize(ui_theme_t theme)
{
    if (theme < 0 || theme >= UI_THEME_COUNT) {
        return UI_THEME_DEFAULT;
    }
    return theme;
}

void ui_theme_set(ui_theme_t theme)
{
    g_active_theme = ui_theme_normalize(theme);
}

ui_theme_t ui_theme_get(void)
{
    return g_active_theme;
}

const char *ui_theme_name(ui_theme_t theme)
{
    switch (ui_theme_normalize(theme)) {
    case UI_THEME_SPORT:
        return "sport";
    case UI_THEME_OEM:
        return "oem";
    case UI_THEME_STEALTH:
        return "stealth";
    default:
        return "sport";
    }
}

const ui_theme_palette_t *ui_theme_current_palette(void)
{
    return &k_theme_palettes[ui_theme_get()];
}

const ui_theme_palette_t *ui_theme_get_palette(ui_theme_t theme)
{
    return &k_theme_palettes[ui_theme_normalize(theme)];
}

lv_color_t ui_theme_rpm_color_for_value(int rpm)
{
    const ui_theme_palette_t *palette = ui_theme_current_palette();
    if (rpm >= 7000) {
        return palette->rpm_color_high;
    }
    if (rpm >= 5000) {
        return palette->rpm_color_mid;
    }
    return palette->rpm_color_low;
}

lv_color_t ui_theme_rpm_color_low(void)
{
    return ui_theme_current_palette()->rpm_color_low;
}

lv_color_t ui_theme_rpm_color_mid(void)
{
    return ui_theme_current_palette()->rpm_color_mid;
}

lv_color_t ui_theme_rpm_color_high(void)
{
    return ui_theme_current_palette()->rpm_color_high;
}

lv_color_t ui_theme_mph_value_color(void)
{
    return ui_theme_current_palette()->mph_value_color;
}

lv_color_t ui_theme_label_color(void)
{
    return ui_theme_current_palette()->primary_label_color;
}

lv_color_t ui_theme_gear_value_color(void)
{
    return ui_theme_current_palette()->gear_value_color;
}

lv_color_t ui_theme_odometer_color(void)
{
    return ui_theme_current_palette()->odometer_color;
}
