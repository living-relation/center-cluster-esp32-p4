#!/usr/bin/env bash
# convert.sh — generate LVGL C font arrays for the center cluster.
# Run once after clone: cd main/ui/fonts && ./convert.sh
# Requires: npm i -g lv_font_conv   (Node.js ≥ 14)
# Source fonts are in <repo-root>/fonts/

set -e
FONTS_DIR="../../../fonts"
OUT_DIR="$(cd "$(dirname "$0")" && pwd)"

run() {
    local name=$1 src=$2 size=$3 bpp=$4 glyphs=$5
    local out="$OUT_DIR/.c"
    [ -f "$out" ] && { echo "  skip $name.c (exists)"; return; }
    echo "  generating $name.c ..."
    lv_font_conv --font "$FONTS_DIR/$src" \
        --size "$size" --bpp "$bpp" --format lvgl \
        --range "$glyphs" \
        -o "$out"
}

echo "TrackCluster center — font conversion"
run aerospace_29  Aerospace.otf  29  4  "0x30-0x39,0x2E"
run aerospace_87  Aerospace.otf  87  4  "0x30-0x39"
run aerospace_188 Aerospace.otf 188  4  "0x30-0x39,0x4E,0x52"
run racehead_14   RaceHead.ttf   14  4  "0x20,0x41-0x5A"
run racehead_22   RaceHead.ttf   22  4  "0x20,0x2D,0x41-0x5A"
run racehead_24   RaceHead.ttf   24  4  "0x31-0x38"
echo "Done. Delete fonts_stub.c to enable real fonts."
