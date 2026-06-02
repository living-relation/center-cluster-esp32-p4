# TrackCluster — Wiring & Pinout (physical install)

**Reference only — NOT flashed.** Lives at the center repo root so it's immediately visible.
Everything below is GPIO/connector-validated against the ESP32-P4 / ESP32-S3
datasheets + errata and the two Waveshare board schematics (June 2026).

Three boards:
- **Center** — Waveshare ESP32-P4-WIFI6-Touch-LCD-XC, 800×800, 40-pin header **J8**.
- **Left / Right** — Waveshare ESP32-S3-Touch-LCD-2.8C, 480×480 (identical boards, different firmware).

---

## 1. Power — 12 V → 5 V buck feeds all three boards

Each board regulates its own 3V3 on-board; **feed them 5 V**, never 3V3 directly.

```
   Vehicle 12 V ──► [12V→5V buck, ≥3 A] ──┬──► Center  5V  (J8 pin 2)   + GND (J8 pin 39)
   (switched/IGN)                          ├──► Left    5V  (5V/VIN pad) + GND
                                           └──► Right   5V  (5V/VIN pad) + GND
```

- **Buck converter:** 12 V in → **5 V** out, **≥3 A** (≈2.5 A peak all-3 with backlights; size up for margin).
  Common ground with the vehicle/ECU.
- **Center 5 V input:** J8 **pin 2 = 5V**, **pin 39 = GND** (or the board's USB-C 5V — but use J8 for the install).
- **Side 5 V input:** each S3 board's **VIN / 5V** pad and **GND** (USB-C VBUS is the same net; the
  PH1.25 2-pin "BAT" connector is for an optional Li-ion only — do not feed 5 V there).
- Add a common-mode choke / 100 µF bulk cap near each board if you see backlight flicker on engine crank.

---

## 2. CAN — ECU ↔ Center (only the center touches CAN)

The ESP32-P4 TWAI controller is logic-level; it needs an **external CAN transceiver**.

```
  Center P4                  CAN transceiver                 Link G4X ECU
  GPIO5 (J8) ──TXD──►        TXD                             CAN bus
  GPIO4 (J8) ◄──RXD──        RXD        CANH ──────────────► CAN Hi
       3V3   ──────►         VCC        CANL ──────────────► CAN Lo
       GND   ──────►         GND        (120 Ω term at each bus end)
```

- **Transceiver:** SN65HVD230 (3.3 V) or isolated ISO1050 / TJA1051T. Power its logic side from the
  center's **3V3**, not 5 V.
- **Bus:** 1 Mbit/s, 120 Ω termination at both physical ends of the CAN backbone (one is usually in
  the ECU; add one 120 Ω at the transceiver end if it's the far end).
- ECU broadcast IDs 0x3E8–0x3EB + status 0x3EE; dash→ECU TX 0x3EC/0x3ED. See `link_g4x_can_setup.*`.

---

## 3. Inter-cluster UART — Center → Left, Center → Right

921600 8N1, one direction used (center transmits; side RX lines reserved are optional). Keep runs
short or twisted; common ground required.

```
  Center P4                          Left S3                 Right S3
  GPIO20 (UART1 TX) ───────────────► GPIO44 (RX)
  GPIO18 (UART1 RX, reserved) ◄────  GPIO43 (TX, reserved)
  GPIO21 (UART2 TX) ─────────────────────────────────────►  GPIO44 (RX)
  GPIO19 (UART2 RX, reserved) ◄───────────────────────────  GPIO43 (TX, reserved)
  GND ───────────────────────────────common───────────────  GND
```

- The S3 boards expose **GPIO43/44 on the on-board UART connector** (silk UART_TXD/UART_RXD).
- **Console note:** GPIO43/44 are the S3's default UART0 console pins. Flash/monitor each side board
  over its **USB-C (USB-Serial-JTAG)** so the inter-cluster link stays clean — the firmware build
  already targets the USB console; don't also drive a serial monitor on GPIO43/44.

---

## 4. Buttons & encoders — Center only (active-low to GND)

All inputs use internal pull-ups; wire the common side to **GND**. 44 kΩ… use the chip pull-ups
(no external resistor needed); add 100 nF across each contact for debounce if noisy.

| Control | Signal | Center GPIO (J8) |
|---|---|---|
| Push-button (ODO / Trip) | to GND | **29** |
| Encoder 1 — Boost map | A / CLK | **30** |
| | B / DT | **31** |
| | push (to GND) | **32** |
| Encoder 2 — TC slip | A / CLK | **49** |
| | B / DT | **50** |
| | push (to GND) | **51** |

---

## 5. Full GPIO reference (all three displays)

### Center — ESP32-P4 (J8 40-pin header)
| Function | GPIO | Notes |
|---|---:|---|
| CAN TX → transceiver | 5 | TWAI |
| CAN RX ← transceiver | 4 | TWAI, 1 Mbit/s |
| UART1 TX → Left | 20 | → Left GPIO44 |
| UART1 RX ← Left (reserved) | 18 | |
| UART2 TX → Right | 21 | → Right GPIO44 |
| UART2 RX ← Right (reserved) | 19 | *(was 20 — fixed; 20 collided with UART1 TX)* |
| Button (ODO/Trip) | 29 | active-low |
| Encoder 1 A/B/SW | 30 / 31 / 32 | |
| Encoder 2 A/B/SW | 49 / 50 / 51 | |
| **Reserved — do not use** | 6,7 (I²C) · 27 (LCD RST) · 37,38 (PSRAM) · 39–44 (microSD) · 34,35,36 (strapping) · DSI pads | |

### Left & Right — ESP32-S3 (identical)
| Function | GPIO | Notes |
|---|---:|---|
| UART RX ← Center TX | 44 | on UART connector |
| UART TX → Center (reserved) | 43 | on UART connector |
| Shared I²C SCL (TCA9554 + GT911) | 7 | drives panel reset/CS via expander |
| Shared I²C SDA | 15 | |
| **Panel RGB (fixed by board)** | R:46,3,8,18,17 · G:14,13,12,11,10,9 · B:5,45,48,47,21 · PCLK 41 · DE 40 · VSYNC 39 · HSYNC 38 · LCD_SDA 1 · LCD_SCK 2 · BL 6 | hard-wired on the Waveshare board — informational only |
| **Reserved — do not use** | 0,3,45,46 (strapping; 3/45/46 also RGB) · 19,20 (USB) · 26–32 (in-package flash/PSRAM) | |

---

## 6. Validation results (datasheet + errata cross-check)

| Item | Result |
|---|---|
| Center CAN 4/5, buttons 29, encoders 30/31/32/49/50/51 | ✅ all on J8, clear of strapping/PSRAM/USB/microSD |
| Center UART pins | ⚠️ **Fixed:** UART2 RX moved 20 → 19 (GPIO20 was assigned to both UART1 TX and UART2 RX) |
| Center "available" list | ⚠️ Annotated: GPIO34/35/36 are **strapping** pins — removed from the free list in Kconfig |
| Side I²C 7/15 | ✅ free, not strapping/USB/flash |
| Side UART 43/44 | ✅ valid (default UART0 console) — **flash via USB-C** so console doesn't fight the link |
| Side RGB uses strapping GPIO3/45/46 | ✅ acceptable — Waveshare-fixed; panel is idle during boot strap sampling |
| ESP32-S3 GPIO19/20 startup glitch (datasheet) | ✅ N/A — those pins are USB, not used for our I/O |
| Errata (S3 + P4) | ✅ no GPIO-level silicon issues affecting this design (entries are cache/secure-boot/PSRAM) |

> Pin values here mirror the firmware `Kconfig.projbuild` of each cluster, which remains authoritative.
> If you change a pin in menuconfig, update this table to match.
