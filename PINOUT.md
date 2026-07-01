# Center cluster — J8 40-pin GPIO header pinout

**Board:** Waveshare **ESP32-P4-WIFI6-Touch-LCD-XC**, 40-pin header **J8**.
This table is transcribed from the **board silkscreen** and cross-checked
against the Waveshare schematic and the firmware pin map in
`main/Kconfig.projbuild`.

## How the pins are numbered

The silkscreen prints **GPIO/function names, not pin numbers**. The physical
pin numbers below follow the standard 2×20 header convention (each column
holds one odd + one even pin) and are anchored to the two hardware facts
already recorded in `WIRING.md`: **J8 pin 2 = 5V** and **J8 pin 39 = GND**.
Testing the column-pair numbering from the `3V3`/`5V` end reproduces both
facts, so numbering starts at that end (pin 1 = 3V3, top row = odd pins,
bottom row = even pins). If you have a Waveshare pin-numbered diagram or a
pin-1 mark on the connector itself, treat that as authoritative over the
inferred numbers here — the **silkscreen names and their GPIO/function
mapping are the part that matters and are not affected by the numbering.**

## Pinout table

Legend: **[wired]** = a wire is landed on this pin in the firmware/harness ·
**[opt]** = pin is assigned in firmware but the harness conductor is optional
and the signal is reserved/not yet active (the inter-cluster RX lines) ·
**[free]** = broken out, unused · **[reserved]** = do not use.

| Pin | Silk | Signal / GPIO | Pin | Silk | Signal / GPIO |
|---:|:---:|---|---:|:---:|---|
| 1 | 3V3 | 3.3 V rail | 2 | 5V | 5 V board input **[wired]** |
| 3 | SDA | GPIO6 — I²C SDA (touch + audio) **[wired]** | 4 | 5V | 5 V board input **[wired]** |
| 5 | SCL | GPIO7 — I²C SCL (touch + audio) **[wired]** | 6 | GND | Ground |
| 7 | 29 | GPIO29 — **ODO/Trip button** **[wired]** | 8 | TXD | ESP32-C6 co-proc UART0 TX — *not a P4 GPIO* |
| 9 | GND | Ground | 10 | RXD | ESP32-C6 co-proc UART0 RX — *not a P4 GPIO* |
| 11 | 21 | GPIO21 — **UART2 TX → RIGHT** **[wired]** | 12 | 22 | GPIO22 — **UART2 RX ← RIGHT** **[opt]** ¹ |
| 13 | 20 | GPIO20 — **UART1 TX → LEFT** **[wired]** | 14 | GND | Ground |
| 15 | 28 | GPIO28 — **UART1 RX ← LEFT** **[opt]** ¹ | 16 | 5 | GPIO5 — **CAN interface** (see §CAN) **[wired]** |
| 17 | 3V3 | 3.3 V rail | 18 | 4 | GPIO4 — **CAN interface** (see §CAN) **[wired]** |
| 19 | 3 | GPIO3 **[free]** | 20 | GND | Ground |
| 21 | 2 | GPIO2 **[free]** | 22 | 35 | GPIO35 — strapping **[reserved]** |
| 23 | 50 | GPIO50 — **Encoder 2 B / DT** **[wired]** | 24 | 36 | GPIO36 — strapping **[reserved]** |
| 25 | GND | Ground | 26 | 49 | GPIO49 — **Encoder 2 A / CLK** **[wired]** |
| 27 | 24 | GPIO24 **[free]** | 28 | 25 | GPIO25 **[free]** |
| 29 | 51 | GPIO51 — **Encoder 2 SW / push** **[wired]** | 30 | GND | Ground |
| 31 | 32 | GPIO32 — **Encoder 1 SW / push** **[wired]** | 32 | 34 | GPIO34 — strapping (JTAG) **[reserved]** |
| 33 | 48 | GPIO48 **[free]** | 34 | GND | Ground |
| 35 | 52 | GPIO52 **[free]** | 36 | 31 | GPIO31 — **Encoder 1 B / DT** **[wired]** |
| 37 | 47 | GPIO47 **[free]** | 38 | 46 | GPIO46 **[free]** |
| 39 | GND | Ground | 40 | 30 | GPIO30 — **Encoder 1 A / CLK** **[wired]** |

¹ **Corrected pins.** The firmware previously defaulted UART1 RX to GPIO18 and
UART2 RX to GPIO19 — **neither of which is routed to J8** (they are unrouted
die pins on the schematic). They are now GPIO28 (UART1 RX) and GPIO22
(UART2 RX), both present on J8. Requires a reflash to take effect. The RX
lines are currently *reserved* (received but not yet decoded by firmware).

## Wired connections summary

| Function | Center GPIO | J8 pin | Notes |
|---|---:|---:|---|
| I²C SDA (touch + audio) | 6 | 3 | fixed by board |
| I²C SCL (touch + audio) | 7 | 5 | fixed by board |
| CAN interface (see §CAN) | 5 | 16 | to transceiver |
| CAN interface (see §CAN) | 4 | 18 | to transceiver |
| UART1 TX → Left | 20 | 13 | → Left GPIO44 (RX) |
| UART1 RX ← Left (reserved) | 28 | 15 | ← Left GPIO43 (TX) |
| UART2 TX → Right | 21 | 11 | → Right GPIO44 (RX) |
| UART2 RX ← Right (reserved) | 22 | 12 | ← Right GPIO43 (TX) |
| ODO / Trip button | 29 | 7 | active-low to GND |
| Encoder 1 (Boost) A / CLK | 30 | 40 | |
| Encoder 1 (Boost) B / DT | 31 | 36 | |
| Encoder 1 (Boost) SW / push | 32 | 31 | active-low to GND |
| Encoder 2 (TC slip) A / CLK | 49 | 26 | |
| Encoder 2 (TC slip) B / DT | 50 | 23 | |
| Encoder 2 (TC slip) SW / push | 51 | 29 | active-low to GND |

## CAN — bus wiring (Hi / Lo)

The ESP32-P4 TWAI controller is **logic level** and cannot drive the CAN bus
directly. GPIO5 and GPIO4 connect to an **external SN65HVD230 transceiver**,
and the **CAN High / CAN Low** differential pair lives on the *transceiver's
bus side* — there are no CAN-Hi/Lo pins on J8 itself.

```
  Center P4 (J8)          SN65HVD230 transceiver          Link G4X ECU
  GPIO5  (pin 16) ──────► CTX (TXD)      CANH ───────────► CAN Hi
  GPIO4  (pin 18) ◄────── CRX (RXD)      CANL ───────────► CAN Lo
  3V3             ──────► VCC            (120 Ω termination at each bus end)
  GND             ──────► GND
```

- **CAN Hi / CAN Lo** are the two bus wires you run to the ECU. Twisted pair,
  120 Ω termination at both physical ends of the backbone (the ECU usually
  provides one).
- Bus rate: **1 Mbit/s** (Link G4X). Do not short CANH and CANL.

## Encoders — wiring and how to determine the correct pins

Each control is a mechanical rotary encoder (EC11-style) with a built-in push
switch: **3 pins** on one side (A, common, B) and **2 pins** for the push
switch. All encoder inputs use the chip's internal pull-ups, so the **common
legs go to GND** (no external resistors needed; add 100 nF across each contact
if you see bounce).

**Center pin groups (from firmware defaults):**
- Encoder 1 (**Boost** map): A=GPIO30, B=GPIO31, SW=GPIO32
- Encoder 2 (**TC** slip angle): A=GPIO49, B=GPIO50, SW=GPIO51

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
