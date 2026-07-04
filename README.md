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

It ships with **bench mode OFF** (live CAN + UART bridge). For a standalone bench sweep, turn on via
*menuconfig → TrackCluster → Bench mode* and reflash. Morning flash steps: **`FLASH_READINESS.md`**.

**Already flashed?** Wiring power, [SN65HVD230 CAN](https://www.waveshare.com/sn65hvd230-can-board.htm),
and UART to the side clusters does **not** require another flash — firmware lives in SPI flash until
you rebuild and reflash after a code change.

### Bench demo video

Recording of bench mode on the center cluster (boot splash, RPM arc, shift LEDs, gear, odometer sweep):

**[`docs/demos/Center_demo.mp4`](docs/demos/Center_demo.mp4)** — open locally after clone, or preview on GitHub by clicking the file.

> **Git LFS:** Demo `.mp4` files are stored with Git LFS. Run `git lfs install` once **before** `git clone`, or `git lfs pull` after clone if the video is missing or only a few kilobytes.

### Key settings (already set in `sdkconfig.defaults`)
| Setting | Value |
|---|---|
| Target | esp32p4 |
| Flash size | **32 MB** (board chip = GD25Q256) |
| PSRAM | Hex, 200 MHz (**required** — the display buffers live here) |
| Bench mode | OFF (live) |
| ESP-IDF | **5.4.2** (use 5.4.x — not 5.3.x, not 5.5+ for v1.x P4 silicon) |

You don’t need to set any baud rate — see the setup guide for why.

---

## Setup & reference documents (in this folder)

| File | What it’s for |
|---|---|
| `SETUP_BEFORE_YOU_BUILD.txt` | Step-by-step VS Code flashing guide for first-timers — flash size, PSRAM, port, baud, bench mode. |
| `WIRING.md` | Physical install: 12 V→5 V power, CAN transceiver, center↔side UART, buttons/encoders, full pin reference + GPIO/silicon validation. |
| `CANBUS-ENCODE-DECODE-REFERENCE.html` | **The** CAN reference — open in any browser. Every channel's ENCODE (ECU transmit) and DECODE (cluster) values side by side, the Link G4X XtremeX + external CAN-Lambda topology, PCLink apply/validation notes, and an embedded machine-readable channel map. |
| `docs/datasheets/LinkCANLambda-manual.pdf` | Link CAN-Lambda module manual (external wideband on the ECU's CAN1 bus). |
| `docs/datasheets/XtremeX-QuickStart-Guide.pdf` | Link G4X XtremeX ECU quick-start guide. |
| `docs/demos/Center_demo.mp4` | Bench-mode screen recording (no CAN/UART required). |
| `docs/harness/` | Wiring diagram (HTML), connector BOM (CSV/MD). |

> These files are reference/setup only — none of them are compiled or flashed. The firmware's
> actual CAN decode lives in `main/canbus.c`.

---

## Audio & desk CAN tools (center only)

- **Boot chime** — four beeps mid-splash (onboard ES8311 speaker).
- **Warning tone** — on rising edge of ECU warn bits from CAN `0x3EE`.
- **CAN sim console** (optional, menuconfig) — inject frames over USB for bench decode tests.

## What’s in the build
```
main/            firmware source (the only thing compiled/flashed)
  ui/            LVGL screens + fonts + images
  audio_alert.c  ES8311 boot + warning tones
  can_sim.c      optional USB CAN inject (menuconfig; off by default)
sdkconfig.defaults   board config (PSRAM, flash, bench mode)
partitions.csv       flash layout
CMakeLists.txt       project file
```
