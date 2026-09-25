# Center cluster — J8 40-pin GPIO header pinout
<!-- Revised 2026-09-25 · feature/center-cluster-harness-design · Cowork: center-cluster doc cleanup + archive · PR# n/a -->
<!-- SOT-REF: repo=living-relation/center-cluster-esp32-p4 path=main/Kconfig.projbuild -->

**Board:** Waveshare **ESP32-P4-WIFI6-Touch-LCD-XC**, 40-pin header **J8**.
This is the **only** J8 pin table in this repo. Any other doc points here;
none restates it.

**Sources:** GPIO assignments = `main/Kconfig.projbuild` (what is flashed).
Silk names and positions = the board silkscreen (Daniel's photo, 2026-09-25,
matched this table 40/40; photo to be committed at
`docs/datasheets/waveshare-esp32-p4-xc-j8-silkscreen.png`) and the
[Waveshare XC schematic](https://files.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-XC/ESP32-P4-WIFI6-Touch-LCD-XC-Schematic.pdf).
I²C GPIOs = Waveshare's
[XC HARDWARE.md](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-XC/blob/main/docs/HARDWARE.md).

## How the pins are numbered

The silkscreen prints **GPIO/function names, not pin numbers**. Pin 1 = the
`3V3` end (standard 2×20: odd pins one row, even pins the other). Confirmed
2026-09-25 against the silkscreen photo and the Waveshare schematic, and by a
working install. Wire by pin number **and** check the silk label at the pin.

## Pinout table

Legend: **[wired]** = a wire is landed on this pin in the firmware/harness ·
**[free]** = broken out, unused · **[reserved]** = do not use.

> The inter-cluster serial link is **TX-only**: the center transmits to the
> Left/Right boards and never receives from them. No center-side UART RX pin
> is used, so GPIO22 is free. (GPIO28 is now the headlight-dimming input.)

| Pin | Silk | Signal / GPIO | Pin | Silk | Signal / GPIO |
|---:|:---:|---|---:|:---:|---|
| 1 | 3V3 | 3.3 V rail | 2 | 5V | 5 V board input **[wired]** |
| 3 | SDA | GPIO7 — I²C SDA (touch + audio), board bus **[reserved]** | 4 | 5V | 5 V board input **[wired]** |
| 5 | SCL | GPIO8 — I²C SCL (touch + audio), board bus **[reserved]** | 6 | GND | Ground |
| 7 | 29 | GPIO29 — **ODO/Trip button** **[wired]** | 8 | TXD | ESP32-C6 co-proc UART0 TX — *not a P4 GPIO* |
| 9 | GND | Ground | 10 | RXD | ESP32-C6 co-proc UART0 RX — *not a P4 GPIO* |
| 11 | 21 | GPIO21 — **UART2 TX → RIGHT** **[wired]** | 12 | 22 | GPIO22 **[free]** |
| 13 | 20 | GPIO20 — **UART1 TX → LEFT** **[wired]** | 14 | GND | Ground |
| 15 | 28 | GPIO28 — **Headlight sense** (active-low) **[wired]** | 16 | 5 | GPIO5 — **CAN interface** (see §CAN) **[wired]** |
| 17 | 3V3 | 3.3 V rail | 18 | 4 | GPIO4 — **CAN interface** (see §CAN) **[wired]** |
| 19 | 3 | GPIO3 **[free]** | 20 | GND | Ground |
| 21 | 2 | GPIO2 — **Encoder 3 SW / push** **[wired]** | 22 | 35 | GPIO35 — strapping **[reserved]** |
| 23 | 50 | GPIO50 — **Encoder 2 B / DT** **[wired]** | 24 | 36 | GPIO36 — strapping **[reserved]** |
| 25 | GND | Ground | 26 | 49 | GPIO49 — **Encoder 2 A / CLK** **[wired]** |
| 27 | 24 | GPIO24 — **Encoder 3 A / CLK** **[wired]** ⚠ USB PHY, move pending | 28 | 25 | GPIO25 — **Encoder 3 B / DT** **[wired]** ⚠ USB PHY, move pending |
| 29 | 51 | GPIO51 — **Encoder 2 SW / push** **[wired]** | 30 | GND | Ground |
| 31 | 32 | GPIO32 — **Encoder 1 SW / push** **[wired]** | 32 | 34 | GPIO34 — strapping (JTAG) **[reserved]** |
| 33 | 48 | GPIO48 **[free]** | 34 | GND | Ground |
| 35 | 52 | GPIO52 **[free]** | 36 | 31 | GPIO31 — **Encoder 1 B / DT** **[wired]** |
| 37 | 47 | GPIO47 **[free]** | 38 | 46 | GPIO46 **[free]** |
| 39 | GND | Ground | 40 | 30 | GPIO30 — **Encoder 1 A / CLK** **[wired]** |

## Wired connections summary

| Function | Center GPIO | J8 pin | Notes |
|---|---:|---:|---|
| I²C SDA (touch + audio) | 7 | 3 | fixed by board — do not wire |
| I²C SCL (touch + audio) | 8 | 5 | fixed by board — do not wire |
| CAN interface (see §CAN) | 5 | 16 | to transceiver |
| CAN interface (see §CAN) | 4 | 18 | to transceiver |
| UART1 TX → Left | 20 | 13 | → Left GPIO44 (RX); TX-only, no center RX |
| UART2 TX → Right | 21 | 11 | → Right GPIO44 (RX); TX-only, no center RX |
| ODO / Trip button | 29 | 7 | active-low to GND |
| Encoder 1 (Boost) A / CLK | 30 | 40 | |
| Encoder 1 (Boost) B / DT | 31 | 36 | |
| Encoder 1 (Boost) SW / push | 32 | 31 | active-low to GND |
| Encoder 2 (TC slip) A / CLK | 49 | 26 | |
| Encoder 2 (TC slip) B / DT | 50 | 23 | |
| Encoder 2 (TC slip) SW / push | 51 | 29 | active-low to GND |
| Encoder 3 (Backlight dim) A / CLK | 24 | 27 | adjusts night level, only while headlights on |
| Encoder 3 (Backlight dim) B / DT | 25 | 28 | |
| Encoder 3 (Backlight dim) SW / push | 2 | 21 | active-low to GND; push resets night level to default |
| Headlight sense | 28 | 15 | **active-low**: switch to GND when headlights on (relay/opto/open-collector). **Do NOT feed +12 V** to this pin |

## CAN — bus wiring (Hi / Lo)

There are no CAN Hi/Lo pins on J8 — only the TWAI logic lines (pins 16/18
above) to an external transceiver. Transceiver hookup, bus rate and
termination: see `WIRING.md` §2.
<!-- SOT-REF: repo=living-relation/center-cluster-esp32-p4 path=WIRING.md anchor=§2 CAN -->
Bus topology and termination for the whole car:
<!-- SOT-REF: repo=living-relation/st185-link-ecu-config path=CAN-BUS-MASTER-DESIGN.md -->
`st185-link-ecu-config/CAN-BUS-MASTER-DESIGN.md`.

## Encoders — wiring and how to determine the correct pins

Each control is a mechanical rotary encoder (EC11-style) with a built-in push
switch: **3 pins** on one side (A, common, B) and **2 pins** for the push
switch. All encoder inputs use the chip's internal pull-ups, so the **common
legs go to GND** (no external resistors needed; add 100 nF across each contact
if you see bounce).

**Center pin groups (from firmware defaults):**
- Encoder 1 (**Boost** map): A=GPIO30, B=GPIO31, SW=GPIO32
- Encoder 2 (**TC** slip angle): A=GPIO49, B=GPIO50, SW=GPIO51
- Encoder 3 (**Backlight dim**): A=GPIO24, B=GPIO25, SW=GPIO2 — adjusts the night
  brightness, and only does anything **while headlights are on**; the push resets
  the night level to the default. Same EC11 wiring (common legs to GND).

**Headlight sense input (backlight dimming):**
- **GPIO28**, **active-low** with the chip's internal pull-up. Wire it to **switch
  to GND** when the headlights/illumination are on (via a relay, opto-isolator, or
  open-collector output). **Do not connect +12 V to the pin** — active-low means no
  divider is needed, but it still requires a ground-switch, not a raw 12 V feed.
- Headlights **on** → all three clusters dim to the night level (the center
  broadcasts the level to the sides over the UART bridge). Headlights **off** →
  full brightness.

### Step 1 — identify the encoder's own pins with a multimeter

1. Set the meter to continuity. On the **3-pin side**, the **centre leg is the
   common (C)**; confirm by finding the leg that shows the switching pattern to
   *both* outer legs as you rotate one detent at a time. Wire **C → GND**.
2. The two **outer legs of the 3-pin side are A and B** (the quadrature
   outputs). Wire them to the encoder's **A/CLK** and **B/DT** GPIOs.
3. The **2-pin side is the push switch** — continuity only while pressed. Wire
   one leg to the **SW** GPIO and the other to **GND**.

### Step 2 — confirm A/B orientation and rotation direction

A and B are interchangeable at wiring time; their order only sets the sign of
the rotation. After flashing:

- Turn the encoder **clockwise**. If the on-screen value / selection moves the
  **wrong way**, **swap the A and B wires** for that encoder (e.g. GPIO30 ↔
  GPIO31 for Encoder 1). No firmware change is needed — it's purely which leg
  lands on the A vs B GPIO.
- Press the knob and confirm the popup **confirms** the selection. If nothing
  happens, the SW leg is on the wrong pin or not grounded.

### Step 3 — confirm which encoder is which

Turn one encoder and watch the display: Encoder 1 opens the **Boost** popup,
Encoder 2 opens the **TC slip-angle** popup (both on the RIGHT display). If
they're reversed, swap the two 3-pin groups (the GPIO30/31/32 set with the
GPIO49/50/51 set) at the connector.

> Pin numbers here mirror the firmware `main/Kconfig.projbuild`, which remains
> authoritative. If you change a pin in `idf.py menuconfig`, update this table.
