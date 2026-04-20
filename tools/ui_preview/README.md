# LVGL UI Preview (No Flash)

This tool renders `ui_Screen1` locally using LVGL and exports still previews for the supported themes.

## What the "4-step list" means

The previous 4 commands were the manual flow:

1. Configure the preview build (`cmake -S ... -B ...`).
2. Compile the preview executable (`cmake --build ...`).
3. Render one theme to a `.ppm` image.
4. Convert `.ppm` to `.png` for easy viewing.

That flow still works, but you now have a one-command shortcut below.

## One-command gallery (recommended)

From repo root:

- `tools/ui_preview/generate_gallery.sh`

This command will:

- build the preview binary
- render sport/oem/stealth images
- convert them to PNG
- write `tools/ui_preview/output/index.html`

By default, the script attempts to open the gallery in your browser (`xdg-open`).

Disable auto-open when needed:

- `tools/ui_preview/generate_gallery.sh --no-open`

## Exact file locations

Tool source files:

- `tools/ui_preview/main_preview.c`
- `tools/ui_preview/CMakeLists.txt`
- `tools/ui_preview/lv_conf.h`
- `tools/ui_preview/lv_conf_preview.h`
- `tools/ui_preview/generate_gallery.sh`

Generated files:

- `tools/ui_preview/output/theme_sport.ppm`
- `tools/ui_preview/output/theme_oem.ppm`
- `tools/ui_preview/output/theme_stealth.ppm`
- `tools/ui_preview/output/theme_sport.png`
- `tools/ui_preview/output/theme_oem.png`
- `tools/ui_preview/output/theme_stealth.png`
- `tools/ui_preview/output/index.html`

## Manual mode (if needed)

From repo root:

- `CC=gcc CXX=g++ cmake -S tools/ui_preview -B tools/ui_preview/build`
- `cmake --build tools/ui_preview/build -j`
- `tools/ui_preview/build/ui_preview sport /tmp/theme_sport.ppm`
- `convert /tmp/theme_sport.ppm /tmp/theme_sport.png`

## Mock Data in Preview

The renderer injects representative CAN-like display values:

- RPM arc: `6200`
- MPH value: `73`
- Gear value: `4`
- Odometer: `012345.6`
