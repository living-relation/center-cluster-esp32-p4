# LVGL UI Preview (No Flash)

This tool renders `ui_Screen1` locally using LVGL and exports a still preview for a selected theme.

## Requirements

- `cmake`
- `make`
- `gcc`/`g++`

## Build

From repo root:

- `CC=gcc CXX=g++ cmake -S tools/ui_preview -B tools/ui_preview/build`
- `cmake --build tools/ui_preview/build -j`

## Generate Preview

- `tools/ui_preview/build/ui_preview sport /tmp/theme_sport.ppm`
- `tools/ui_preview/build/ui_preview oem /tmp/theme_oem.ppm`
- `tools/ui_preview/build/ui_preview stealth /tmp/theme_stealth.ppm`

The utility writes a PPM image, which can be converted to PNG with ImageMagick:

- `convert /tmp/theme_sport.ppm /tmp/theme_sport.png`

## Mock Data in Preview

The renderer injects representative CAN-like display values:

- RPM arc: `6200`
- MPH value: `73`
- Gear value: `4`
- Odometer: `012345.6`
