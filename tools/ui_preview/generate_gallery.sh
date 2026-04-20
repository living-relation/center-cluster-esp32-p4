#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PREVIEW_DIR="$ROOT_DIR/tools/ui_preview"
BUILD_DIR="$PREVIEW_DIR/build"
OUTPUT_DIR="$PREVIEW_DIR/output"
OPEN_BROWSER=1

print_usage() {
  echo "Usage: $0 [--no-open]"
  echo
  echo "Generates all theme previews and writes:"
  echo "  - $OUTPUT_DIR/theme_sport.png"
  echo "  - $OUTPUT_DIR/theme_oem.png"
  echo "  - $OUTPUT_DIR/theme_stealth.png"
  echo "  - $OUTPUT_DIR/index.html"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --open)
      OPEN_BROWSER=1
      shift
      ;;
    --no-open)
      OPEN_BROWSER=0
      shift
      ;;
    -h|--help)
      print_usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      print_usage
      exit 1
      ;;
  esac
done

mkdir -p "$OUTPUT_DIR"

CC=gcc CXX=g++ cmake -S "$PREVIEW_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j4

"$BUILD_DIR/ui_preview" sport "$OUTPUT_DIR/theme_sport.ppm"
"$BUILD_DIR/ui_preview" oem "$OUTPUT_DIR/theme_oem.ppm"
"$BUILD_DIR/ui_preview" stealth "$OUTPUT_DIR/theme_stealth.ppm"

convert "$OUTPUT_DIR/theme_sport.ppm" "$OUTPUT_DIR/theme_sport.png"
convert "$OUTPUT_DIR/theme_oem.ppm" "$OUTPUT_DIR/theme_oem.png"
convert "$OUTPUT_DIR/theme_stealth.ppm" "$OUTPUT_DIR/theme_stealth.png"

cat > "$OUTPUT_DIR/index.html" <<'HTML'
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>LVGL Theme Gallery</title>
    <style>
      :root {
        color-scheme: dark;
      }
      body {
        margin: 0;
        font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif;
        background: #0d1117;
        color: #e6edf3;
      }
      header {
        padding: 20px;
        border-bottom: 1px solid #30363d;
      }
      h1 {
        margin: 0 0 8px;
        font-size: 22px;
      }
      p {
        margin: 0;
        color: #9da7b3;
      }
      main {
        padding: 20px;
      }
      .grid {
        display: grid;
        gap: 20px;
        grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      }
      figure {
        margin: 0;
        border: 1px solid #30363d;
        border-radius: 12px;
        overflow: hidden;
        background: #161b22;
      }
      figcaption {
        padding: 12px 14px;
        font-weight: 600;
        border-bottom: 1px solid #30363d;
      }
      img {
        width: 100%;
        height: auto;
        display: block;
      }
      .meta {
        margin-top: 18px;
        font-size: 13px;
        color: #9da7b3;
      }
      code {
        background: #0b0f14;
        border: 1px solid #30363d;
        border-radius: 6px;
        padding: 2px 6px;
      }
    </style>
  </head>
  <body>
    <header>
      <h1>LVGL Theme Gallery</h1>
      <p>Generated with local host renderer from <code>ui_Screen1</code>.</p>
    </header>
    <main>
      <section class="grid">
        <figure>
          <figcaption>Sport</figcaption>
          <img src="./theme_sport.png" alt="Sport theme preview" />
        </figure>
        <figure>
          <figcaption>OEM</figcaption>
          <img src="./theme_oem.png" alt="OEM theme preview" />
        </figure>
        <figure>
          <figcaption>Stealth</figcaption>
          <img src="./theme_stealth.png" alt="Stealth theme preview" />
        </figure>
      </section>
      <p class="meta">
        Mock CAN-style values in preview: RPM=6200, MPH=73, Gear=4, Odometer=012345.6
      </p>
    </main>
  </body>
</html>
HTML

echo "Generated gallery:"
echo "  $OUTPUT_DIR/index.html"

if [ "$OPEN_BROWSER" -eq 1 ]; then
  if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$OUTPUT_DIR/index.html" >/dev/null 2>&1 || true
    echo "Attempted to open in browser with xdg-open."
  else
    echo "xdg-open not found; open manually in your browser:"
    echo "  file://$OUTPUT_DIR/index.html"
  fi
fi
