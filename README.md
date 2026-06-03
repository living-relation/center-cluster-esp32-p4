# TrackCluster — Center Cluster (ESP32-P4)

Firmware for the **center** instrument-cluster display: an 800×800 round MIPI-DSI panel on a
**Waveshare ESP32-P4-WIFI6-Touch-LCD-XC**. The center is the system's brain — it reads the
**Link G4X ECU** over CAN, renders the tach / shift-lights / gear / odometer, and relays a data
subset to the two side screens over UART.

This is one of three projects:
- **center-cluster-esp32-p4** ← (this repo) tach + shift LEDs + gear + odometer
- **left-side-cluster-esp32s3** — speedo + oil/fuel mini-gauges
- **right-side-cluster-esp32s3** — lambda + boost/temp bar gauges + ECU warnings

---

## Flash it (quick start in VS Code)

Full, beginner-friendly walkthrough: **[`SETUP_BEFORE_YOU_BUILD.txt`](SETUP_BEFORE_YOU_BUILD.txt)** —
read that first if you're new to VS Code / ESP-IDF.

Short version:
1. Install **VS Code** + the official **Espressif IDF** extension; run
   *Command Palette → “ESP-IDF: Configure ESP-IDF Extension” → Express → 5.4.2*.
   (Use **ESP-IDF 5.4.x**. Do **not** use 5.3.x, and do **not** use 5.5+ — 5.5 dropped support
   for early ESP32-P4 **v1.x** silicon, which is what this board has. 5.4.2 is the tested version.)
2. **File → Open Folder →** this repo.
3. Set target = **esp32p4** (chip icon on the bottom bar).
4. Plug the board in via USB-C and pick the **COM port** on the bottom bar.
5. Click **Build → Flash → Monitor** (the flame icon does all three).

It ships in **BENCH MODE** — the screen runs a self-test demo with nothing else wired up, so you
can confirm the display works on the bench. Turn it off later via
*menuconfig → TrackCluster → Bench mode* (or ask and it’ll be switched off).

### Key settings (already set in `sdkconfig.defaults`)
| Setting | Value |
|---|---|
| Target | esp32p4 |
| Flash size | **32 MB** (board chip = GD25Q256) |
| PSRAM | Hex, 200 MHz (**required** — the display buffers live here) |
| Bench mode | ON (self-test) |
| ESP-IDF | **5.4.2** (use 5.4.x — not 5.3.x, not 5.5+ for v1.x P4 silicon) |

You don’t need to set any baud rate — see the setup guide for why.

---

## Setup & reference documents (in this folder)

| File | What it’s for |
|---|---|
| `SETUP_BEFORE_YOU_BUILD.txt` | Step-by-step VS Code flashing guide for first-timers — flash size, PSRAM, port, baud, bench mode. |
| `WIRING.md` | Physical install: 12 V→5 V power, CAN transceiver, center↔side UART, buttons/encoders, full pin reference + GPIO/silicon validation. |
| `CANBUS-LINK-G4X-CONFIG.md` | Human-readable CAN configuration — every ECU broadcast/receive frame, byte map, scaling, and the PCLink “Custom CAN” setup for the Link G4X. |
| `link_g4x_can_setup.lcs` | Importable PCLink CAN Setup file (CAN → Setup → File → Open). |
| `link_g4x_can_setup.json` | Human-readable canonical spec of that CAN config. |

> These files are reference/setup only — none of them are compiled or flashed. The firmware’s own
> CAN decode map is `main/protocols/link_g4x.json` (that one **is** part of the build).

---

## What’s in the build
```
main/            firmware source (the only thing compiled/flashed)
  ui/            LVGL screens + fonts + images
  protocols/     link_g4x.json — CAN decode map (code)
sdkconfig.defaults   board config (PSRAM, flash, bench mode)
partitions.csv       flash layout
CMakeLists.txt       project file
```
