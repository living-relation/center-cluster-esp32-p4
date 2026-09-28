# TrackCluster — Wiring & Pinout (physical install)
<!-- Revised 2026-09-25 · feature/center-cluster-harness-design · Cowork: center-cluster doc cleanup + archive · PR# n/a -->
<!-- Revised 2026-09-27 · chore/wiring-side-sot-refs · Cowork: S3 pin table replaced with SOT-REF pointers · PR# n/a -->
<!-- Revised 2026-09-27 · feature/center-cluster-dupont-harness · Cowork: Dupont-only, buck soldered direct · PR# n/a -->

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
- **Connectors:** Dupont-style 2.54 housings at the boards only. Everything else is a wire-to-wire splice. Solder the buck's VIN, VOUT and GND leads straight to its pads (no connector on the buck), and give each board its own 5 V wire from the 5 V splice.
- **Center 5 V input:** J8 **pin 2 = 5V**, **pin 39 = GND** (or the board's USB-C 5V — but use J8 for the install).
- **J8 pin numbers:** `PINOUT.md` only. Wires land directly on J8 (no adapter).
  **Parts to buy:** `docs/harness/PURCHASE-LIST.csv`. The old harness drawing
  was archived 2026-09-25 (wrong pin numbers) — see `archive/2026-09-25-wiring/`.
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
- **Waveshare [SN65HVD230 CAN Board](https://www.waveshare.com/sn65hvd230-can-board.htm):** passive
  hardware — **no firmware or programming**. Wire **VCC→3V3**, **GND→GND**, **CTX→GPIO5 (TWAI TX)**,
  **CRX→GPIO4 (TWAI RX)**, **CANH/CANL** to the ECU bus. Supports up to **1 Mbit/s** (matches Link G4X).
  Do not short CANH and CANL.
- **Bus:** 1 Mbit/s. CAN H/L wiring and termination are owned by the ST185 CAN harness (the
  installed transceiver has a 120 Ω jumper, set per that design).
  <!-- SOT-REF: repo=living-relation/st185-link-ecu-config path=docs/harness/rebuild/ST185-CAN.harness -->
- ECU broadcast IDs 0x3E8–0x3EB + status 0x3EE; dash→ECU TX 0x3EC/0x3ED. See `CANBUS-ENCODE-DECODE-REFERENCE.html`.

---

## 3. Inter-cluster UART — Center → Left, Center → Right

921600 8N1, **TX-only** — the center transmits to each side board and never
receives from them. There is no center-side RX pin or wire. Keep runs short or
twisted; common ground required.

```
  Center P4                          Left S3                 Right S3
  GPIO20 (UART1 TX) ───────────────► GPIO44 (RX)
  GPIO21 (UART2 TX) ─────────────────────────────────────►  GPIO44 (RX)
  GND ───────────────────────────────common───────────────  GND
```

- Only the two TX lines + common GND are wired. The side boards' GPIO43/TX is
  left unconnected on the center end.
- The S3 boards expose **GPIO44 as the RX input** on the on-board UART connector.
- **Console note:** GPIO43/44 are the S3's default UART0 console pins. Flash/monitor each side board
  over its **USB-C (USB-Serial-JTAG)** so the inter-cluster link stays clean — the firmware build
  already targets the USB console; don't also drive a serial monitor on GPIO43/44.

---

## 4. Buttons & encoders — Center only (active-low to GND)

Pins, GPIOs, encoder wiring steps and the headlight-sense input rules live in
`PINOUT.md` (Wired connections summary + Encoders section). Not repeated here.
<!-- SOT-REF: repo=living-relation/center-cluster-esp32-p4 path=PINOUT.md anchor=Wired connections summary -->

---

## 5. Full GPIO reference (all three displays)

### Center — ESP32-P4 (J8 40-pin header)
See `PINOUT.md` (J8 table) and `main/Kconfig.projbuild` (firmware pin map and
board-reserved GPIOs). Not repeated here.
<!-- SOT-REF: repo=living-relation/center-cluster-esp32-p4 path=PINOUT.md -->
<!-- SOT-REF: repo=living-relation/center-cluster-esp32-p4 path=main/Kconfig.projbuild -->

### Left & Right - ESP32-S3 (identical)
No pin data is kept here. I2C and UART pins live in each side repo's Kconfig; panel/RGB pins are fixed by the Waveshare ESP32-S3-Touch-LCD-2.8C schematic.
<!-- SOT-REF: repo=living-relation/left-side-cluster-esp32s3 path=main/Kconfig.projbuild -->
<!-- SOT-REF: repo=living-relation/right-side-cluster-esp32s3 path=main/Kconfig.projbuild -->

---

## 6. Validation results (datasheet + errata cross-check)

| Item | Result |
|---|---|
| Center CAN 4/5, buttons 29, encoders 30/31/32/49/50/51 | ✅ all on J8, clear of strapping/PSRAM/USB/microSD |
| Center UART link | ✅ **TX-only:** center transmits on GPIO20 (Left) / GPIO21 (Right); no center RX pin is claimed. The previously "reserved" RX pins were removed entirely — the side-board-TX → center-RX link is not used. |
| Center Encoder 3 A/B | ⚠️ Accepted 2026-09-25: shares pins with the "USB" USB-C port. Flash via "USB TO UART" only. See `PINOUT.md` USB note. |
| Center "available" list | ⚠️ Annotated: GPIO34/35/36 are **strapping** pins — removed from the free list in Kconfig |
| Side I²C 7/15 | ✅ free, not strapping/USB/flash |
| Side UART RX 44 | ✅ valid (default UART0 console pin) — **flash via USB-C** so console doesn't fight the link. Only GPIO44/RX is used; GPIO43/TX is left unconnected on the center end. |
| Side RGB uses strapping GPIO3/45/46 | ✅ acceptable — Waveshare-fixed; panel is idle during boot strap sampling |
| ESP32-S3 GPIO19/20 startup glitch (datasheet) | ✅ N/A — those pins are USB, not used for our I/O |
| Errata (S3 + P4) | ✅ no GPIO-level silicon issues affecting this design (entries are cache/secure-boot/PSRAM) |

