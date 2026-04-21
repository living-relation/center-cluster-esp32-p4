# Center Cluster Layout Procedure (SquareLine Studio + LVGL + CAN + UART)

Beginner-friendly guide (8th grade reading level) for your setup:

- Main display: 3.4" round Waveshare ESP32-P4 touchscreen
- Two smaller ESP32 displays over UART
- Modes: Boot, Street, Race, and Warning overlay on Street/Race

---

## 1) What this guide does

This guide shows you how to:

1. Design your screens in SquareLine Studio (click by click)
2. Export those screens into your project
3. Connect live CAN channels (RPM, MAP, Lambda, Gear, warnings)
4. Send data to your 2 small UART displays

---

## 2) Before you start

You need:

- SquareLine Studio (LVGL 8.3.x project)
- Your logo image (Toyota/custom)
- Your custom fonts (`.ttf` / `.otf`)
- This repo checked out locally

Current project uses:

- LVGL 8.3.11
- SquareLine-generated files in `main/tach_ui`
- CAN protocol JSON files in `main/canbus/protocols`

---

## 3) Create a new SquareLine project

1. Open **SquareLine Studio**
2. Click **New Project**
3. Fill in:
   - **Project Name**: `center_cluster_v2`
   - **Resolution**: `800 x 800`
   - **Color Depth**: `16-bit`
   - **LVGL Version**: `8.3.x`
4. Click **Create**

Why `800x800`:
- Your current UI code is based on an 800 base resolution with scaling support.

---

## 4) Import assets (logo + custom fonts)

### 4.1 Import boot logo/background

1. In left panel, click **Assets**
2. Click **+ Image**
3. Select your Toyota/custom image
4. Confirm it appears in asset list

### 4.2 Import custom fonts

1. Open **Fonts**
2. Click **+ Add Font**
3. Select your font file
4. Add sizes you need (example): `32`, `48`, `72`, `120`
5. Include glyphs:
   - `0123456789`
   - `. : -`
   - `N GEAR MPH MAP LAMBDA E F`
6. Repeat for each custom font
7. Save project

Tip:
- If text shows empty boxes later, the font glyph list is missing characters.

---

## 5) Create screens (Boot, Street, Race)

In the **Screen Tree**:

1. Add screen: `BootScreen`
2. Add screen: `StreetScreen`
3. Add screen: `RaceScreen`

Keep object names clear and consistent. This makes code wiring easy.

---

## 6) Build the Boot screen (fade out after boot)

1. Select `BootScreen`
2. Add a full-screen black background panel
3. Add an **Image** widget in center
4. Set image source to your logo asset
5. (Optional) Add glow/shadow style
6. Add fade animation:
   - Logo fade in
   - Then transition to Street screen using fade

Recommended object names:

- `boot_bg`
- `boot_logo`

Behavior goal:
- Boot screen appears during startup, then fades into Street screen.

---

## 7) Build the Street screen (your requested layout)

## 7.1 Tachometer (top half segmented arc)

1. Select `StreetScreen`
2. Add a **Meter** widget centered toward top
3. Configure scale:
   - Min: `0`
   - Max: `8000`
   - Redline start: `7000`
4. Add major labels:
   - `0 1 2 3 4 5 6 7 8`
5. Style arc colors:
   - White in low range
   - Transition to red near redline

Name it:
- `street_tach_meter`

## 7.2 Fuel arc (bottom)

1. Add a smaller/thinner arc or meter near bottom
2. Outline color: solid white
3. Add tick markers/labels:
   - `E`, `1/2`, `F`
4. Add fill indicator for fuel level

Name it:
- `street_fuel_arc`

## 7.3 MAP digital gauge (center)

1. Add large text label in center
2. Set placeholder text to `00`
3. Use custom font

Name it:
- `street_map_value`

## 7.4 Lambda digital gauge (below MAP)

1. Add slightly smaller label below MAP
2. Placeholder text like `1.00`

Name it:
- `street_lambda_value`

## 7.5 Shift lights row (6 square LEDs)

1. Add horizontal container below Lambda and above fuel arc
2. Add 6 square widgets inside container
3. Color them:
   - 1-2: bright green
   - 3-4: bright yellow
   - 5-6: bright red

Names:
- `street_shift_led_1` ... `street_shift_led_6`

## 7.6 Warning overlay for Street

1. Add full-screen semi-transparent panel
2. Add warning text (optional): `ENGINE PROTECT`
3. Set hidden by default

Name:
- `street_warning_overlay`

---

## 8) Build the Race screen (your requested layout)

## 8.1 Tachometer (wider, brighter, no labels)

1. Add tach meter in same area as Street
2. Make arc thicker/more visible
3. Remove number labels from arc scale
4. Use brighter segment colors
5. Add digital RPM label under arc

Names:
- `race_tach_meter`
- `race_rpm_value`

## 8.2 Fuel arc (same location/style)

1. Add same style as Street fuel arc
2. Keep E / 1/2 / F marks

Name:
- `race_fuel_arc`

## 8.3 Gear indicator (center)

1. Add large bright center label
2. Placeholder: `N` or `1`

Name:
- `race_gear_value`

## 8.4 Bigger shift lights

1. Add LED row below gear indicator
2. Make LEDs larger than Street
3. Add more LEDs only if there is room

Names:
- `race_shift_led_1` ... `race_shift_led_n`

## 8.5 Warning overlay for Race

1. Add full-screen warning overlay
2. Set hidden by default

Name:
- `race_warning_overlay`

---

## 9) Add mode switching

Choose one (or both):

- Button toggle (Street <-> Race)
- CAN-controlled mode field

In SquareLine:

1. Add button `btn_to_street` on Race screen
2. Add button `btn_to_race` on Street screen
3. Add click event:
   - Action: Change Screen
   - Target: Street or Race

---

## 10) Export generated UI files into this project

1. In SquareLine, open **Project Settings** -> **Export**
2. Set export folder to:
   - `/workspace/main/tach_ui`
3. Click **Export** / **Generate**

Expected generated files include:

- `main/tach_ui/ui.c`
- `main/tach_ui/ui.h`
- `main/tach_ui/screens/*.c`
- `main/tach_ui/screens/*.h`
- image/font assets

---

## 11) Wire boot + modes + warning in code

After export, add runtime code (outside generated files where possible):

1. Boot flow:
   - Create/show boot screen
   - Fade into Street screen after startup
2. Mode state:
   - `MODE_STREET`
   - `MODE_RACE`
3. Warning state:
   - If any protect condition is active:
     - show warning overlay on active mode
     - flash background colors (timer-based)

Good practice:
- Keep custom logic in separate files (example: `ui_runtime.c`, `warning_mode.c`)
- Avoid heavy edits inside generated SquareLine files

---

## 12) CAN mapping (important)

Your CAN system loads protocol JSON files from:

- `main/canbus/protocols/*.json`

And maps known names to live values.

To support your screens, map and use:

- `rpm`
- `speed`
- `map`
- `lambda` / `lambda1`
- `gear`
- protect flags:
  - `knock`, `knocking`
  - `fuel_cut`, `ignition_cut`
  - `engine_limit_active`
  - pressure warning flags

Then in your gauge update function:

- Street:
  - update tach arc + labels + MAP + Lambda + shift LEDs + fuel arc
- Race:
  - update wide tach + rpm text + gear + larger shift LEDs + fuel arc
- Both:
  - evaluate protect flags -> warning mode on/off

---

## 13) UART packet for your 2 smaller displays

Your main unit already transmits to 2 UART ports.

Update packet struct if needed to include:

- mode (`street`/`race`)
- warning active
- rpm
- map
- lambda
- gear
- shift LED bitmask/state

Then update both receiver firmwares on small displays to parse the new packet format.

---

## 14) Testing checklist (in order)

1. Power up -> Boot screen shows logo
2. Boot fades into Street screen
3. Street/Race mode switch works
4. RPM, MAP, Lambda, Gear update from CAN
5. Fuel arc updates correctly
6. Shift LEDs respond to RPM
7. Trigger protect condition -> warning overlay + flashing background
8. Both small UART displays receive and show expected data

---

## 15) Naming map (copy this exactly)

Use these object IDs in SquareLine so code is easy to wire:

- `BootScreen`, `StreetScreen`, `RaceScreen`
- `boot_logo`
- `street_tach_meter`
- `street_fuel_arc`
- `street_map_value`
- `street_lambda_value`
- `street_shift_led_1..6`
- `street_warning_overlay`
- `race_tach_meter`
- `race_rpm_value`
- `race_fuel_arc`
- `race_gear_value`
- `race_shift_led_1..n`
- `race_warning_overlay`
- `btn_to_street`, `btn_to_race`

---

## 16) Common mistakes to avoid

1. **Wrong LVGL version**: use 8.3.x to match project.
2. **Wrong color depth**: keep 16-bit.
3. **Missing font glyphs**: include all characters you display.
4. **Editing generated files heavily**: put logic in separate runtime files.
5. **Changing UART packet without receiver updates**: update both ends together.
6. **No warning debounce**: flashing can chatter if protect flag is noisy; add debounce/hysteresis.

---

## 17) Short “do this now” summary

1. Build Boot/Street/Race screens in SquareLine with the names above.
2. Export to `main/tach_ui`.
3. Add runtime logic for:
   - mode switching
   - warning flashing overlays
4. Extend CAN mappings for MAP/Lambda/Gear/protect flags.
5. Extend UART payload for your two small displays.
6. Flash and run the test checklist.

