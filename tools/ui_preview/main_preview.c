#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl.h"

#include "ui.h"
#include "ui_theme.h"

#define PREVIEW_RES 800
#define TICK_MS 16
#define CANVAS_BUF_SIZE (PREVIEW_RES * PREVIEW_RES * 4)

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_color_t buf_1[PREVIEW_RES * 80];
static uint32_t g_fake_tick_ms = 0;

uint32_t preview_get_tick_ms(void)
{
    return g_fake_tick_ms;
}

static void preview_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    LV_UNUSED(area);
    LV_UNUSED(color_p);
    lv_disp_flush_ready(drv);
}

static void register_display(void)
{
    lv_disp_drv_init(&disp_drv);

    lv_disp_draw_buf_init(&draw_buf, buf_1, NULL, PREVIEW_RES * 80);

    disp_drv.hor_res = PREVIEW_RES;
    disp_drv.ver_res = PREVIEW_RES;
    disp_drv.flush_cb = preview_flush_cb;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);
}

static uint8_t rgb565_to_r8(lv_color_t color)
{
    return (uint8_t)((color.ch.red * 255) / 31);
}

static uint8_t rgb565_to_g8(lv_color_t color)
{
    return (uint8_t)((color.ch.green * 255) / 63);
}

static uint8_t rgb565_to_b8(lv_color_t color)
{
    return (uint8_t)((color.ch.blue * 255) / 31);
}

static void save_ppm_from_rgb565(const char *path, const lv_color_t *pixels)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open output file: %s\n", path);
        exit(1);
    }

    fprintf(fp, "P6\n%d %d\n255\n", PREVIEW_RES, PREVIEW_RES);
    for (int i = 0; i < PREVIEW_RES * PREVIEW_RES; i++) {
        const lv_color_t color = pixels[i];
        uint8_t rgb[3] = {
            rgb565_to_r8(color),
            rgb565_to_g8(color),
            rgb565_to_b8(color),
        };
        fwrite(rgb, 1, 3, fp);
    }

    fclose(fp);
}

static void set_mock_values(void)
{
    lv_arc_set_value(ui_rpm_arc, 6200);
    lv_label_set_text(ui_label_mph_value, "73");
    lv_label_set_text(ui_label_gear_value, "4");
    lv_label_set_text(ui_label_odometer_value, "012345.6");

    lv_obj_set_style_arc_color(ui_rpm_arc, ui_theme_rpm_color_for_value(6200), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_label_gear_value, ui_theme_gear_value_color(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_label_odometer_value, ui_theme_odometer_color(), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void render_theme(ui_theme_t theme, const char *output_path)
{
    ui_theme_set(theme);
    set_mock_values();

    for (int i = 0; i < 2; i++) {
        g_fake_tick_ms += TICK_MS;
        lv_timer_handler();
    }

    lv_obj_t *root = lv_scr_act();
    uint32_t snapshot_size = lv_snapshot_buf_size_needed(root, LV_IMG_CF_TRUE_COLOR);
    uint8_t *snapshot_buf = malloc(snapshot_size);
    if (!snapshot_buf) {
        fprintf(stderr, "Failed to allocate snapshot buffer (%u bytes)\n", snapshot_size);
        exit(1);
    }

    lv_img_dsc_t snapshot = {0};
    if (lv_snapshot_take_to_buf(root, LV_IMG_CF_TRUE_COLOR, &snapshot, snapshot_buf, snapshot_size) != LV_RES_OK) {
        free(snapshot_buf);
        fprintf(stderr, "Failed to capture LVGL snapshot for theme %s\n", ui_theme_name(theme));
        exit(1);
    }

    save_ppm_from_rgb565(output_path, (const lv_color_t *)snapshot.data);
    free(snapshot_buf);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <sport|oem|stealth> <output.ppm>\n", argv[0]);
        return 1;
    }

    const char *theme_arg = argv[1];
    const char *output_path = argv[2];
    ui_theme_t theme = UI_THEME_SPORT;

    if (strcmp(theme_arg, "sport") == 0) {
        theme = UI_THEME_SPORT;
    } else if (strcmp(theme_arg, "oem") == 0) {
        theme = UI_THEME_OEM;
    } else if (strcmp(theme_arg, "stealth") == 0) {
        theme = UI_THEME_STEALTH;
    } else {
        fprintf(stderr, "Unknown theme: %s\n", theme_arg);
        return 1;
    }

    lv_init();
    register_display();

    ui_theme_set(theme);
    ui_init();
    render_theme(theme, output_path);
    ui_destroy();

    return 0;
}
