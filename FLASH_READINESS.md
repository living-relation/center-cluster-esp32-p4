# Center cluster — flash readiness

**Source tree (flash from here):** `C:\projects\center-cluster-esp32-p4`  
**Git HEAD:** run `git rev-parse --short HEAD` after pull  
**Mode:** bench **OFF** — live CAN + UART bridge to sides (`# CONFIG_TC_BENCH_MODE is not set`)

> **⚠️ Reflash needed (CAN pressure/MAP fix, 2026-06-28):** CAN `0x3E9` oil & fuel pressure are **2 bytes** (oil = bytes 3-4, fuel = bytes 5-6) so they read past 37 PSI, and boost is derived from **MAP** (absolute). A board flashed **before 2026-06-28** must be reflashed for these to take effect. Full channel map: `CANBUS-ENCODE-DECODE-REFERENCE.html`.

## Unwired bench — flash this board alone

Plug **only** the center cluster via USB-C. ECU CAN and side UART are not required to verify the flash.

## Morning checklist

1. **ESP-IDF 5.4.2 only** — do **not** use 5.5+ on this P4 v1.x board
2. **USB** — center board only; note the COM port it enumerates as (varies per PC/session)
3. **Target** — `esp32p4` (set once via VS Code chip icon or `idf.py set-target esp32p4`)
4. **Build:**
   ```powershell
   cd C:\projects\center-cluster-esp32-p4
   .\scripts\prepare_flash.ps1
   ```
5. **Flash:** (pass the COM port the board enumerated as)
   ```powershell
   .\scripts\flash_cluster.ps1 -Port <your COM port>
   ```
6. **Confirm `build/flash_args`** contains `0x2000 bootloader` — required for P4 boot

## What to expect (bench off, unwired)

- Toyota splash (mid-splash **boot chime** on onboard speaker)
- RPM arc / shift LEDs / gear / odometer at **0** — **no demo sweep**
- ECU CAN and side UART not connected yet; that is fine on the bench
- Serial log should **not** show `BENCH MODE`
- **Warning tone** plays when ECU sets `g_dash.warn` (CAN `0x3EE` bytes 0–5)

## Optional bench CAN inject (menuconfig)

*TrackCluster → Enable CAN frame injection over USB console* — REPL commands `can <id> <hex>` and
`warn_test` for desk testing without a transceiver. **Pinned off** in the default car build
(`CONFIG_TC_CAN_SIM_CONSOLE=n` in `sdkconfig.defaults`); only enable it deliberately for bench work.

## Settings (already in `sdkconfig.defaults`)

| Item | Value |
|------|--------|
| Target | esp32p4 |
| Flash | 16 MB (menu may show 16 MB; module is 32 MB — do not exceed chip in esptool) |
| PSRAM | Hex 200 MHz (**required** for 800×800) |
| Bootloader offset | **0x2000** |
| Bench mode | **off** |
| CAN TX / RX GPIO | **5 / 4** → [SN65HVD230 board](https://www.waveshare.com/sn65hvd230-can-board.htm) |

## After you wire the car — reflash?

**No**, if each board already has the latest **bench-off** firmware flashed. Wiring 12 V, CAN,
and UART does not change flash contents. Reflash only when you pull new firmware or change
menuconfig (e.g. bench mode, CAN sim console).
