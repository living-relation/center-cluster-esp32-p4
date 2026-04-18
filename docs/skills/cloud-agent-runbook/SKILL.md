# Cloud Agent Runbook: ESP32-P4 Digital Gauge Cluster

Use this skill whenever a Cloud agent needs to build, flash, test, or modify this
codebase. Read it before touching any code, and update it whenever you discover new
tricks or runbook knowledge.

---

## 1. Codebase at a Glance

| Item | Detail |
|------|--------|
| Platform | ESP32-P4 (`esp32p4`) |
| Framework | ESP-IDF (CMake) — version pinned to **5.5.0** in `dependencies.lock`; `sdkconfig.defaults` targets 5.4.0 |
| Language | C (primary), C++ (`gps_wrapper.cpp`) |
| UI | LVGL 8.4.x via Waveshare BSP (`waveshare/esp32_p4_wifi6_touch_lcd_xc`) |
| Dep manager | ESP Component Manager (`idf_component.yml`, `dependencies.lock`) |
| Build system | `idf.py` (ESP-IDF CLI) — no `make`, no npm, no pip |
| CI | None |
| Tests | No automated test suite. All testing is manual on hardware. |
| Flash target | Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C (or 4in variant) |

---

## 2. Environment Setup

### 2.1 Prerequisites (install once per VM)

```bash
# Install ESP-IDF 5.5 — adjust path as needed
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf
git checkout v5.5
git submodule update --init --recursive
./install.sh esp32p4

# Source the environment (must be done in every new shell)
. ~/esp/esp-idf/export.sh
```

> The `.vscode/settings.json` in this repo assumes an IDF install at
> `/Users/tway/esp/esp-idf` — update that if running on a different machine.
> The `idf.py` CLI does not care about this file; it only reads `$IDF_PATH`.

### 2.2 Verify the environment is active

```bash
idf.py --version          # should print v5.x.x
echo $IDF_PATH            # should be non-empty
idf.py set-target esp32p4 # sets target in sdkconfig; safe to re-run
```

### 2.3 Resolve managed components

On a fresh checkout (or after editing `idf_component.yml` / `dependencies.lock`):

```bash
idf.py update-dependencies
```

This fetches LVGL, the Waveshare BSP, audio player, etc. into
`managed_components/` (git-ignored, so always safe to delete and re-fetch).

---

## 3. Build

```bash
# From repo root (with IDF env sourced)
idf.py build
```

Artifacts land in `build/`. Expect ~60–90 s on first build.

**Common build errors and fixes:**

| Error | Fix |
|-------|-----|
| `IDF_PATH not set` | Run `. ~/esp/esp-idf/export.sh` |
| `Target mismatch` | Run `idf.py set-target esp32p4` then rebuild |
| `Component not found` | Run `idf.py update-dependencies` |
| `bootloader requires chip revision ≥ v3.1` | Change minimum supported chip rev in menuconfig (see README) |
| `ADC_ATTEN redefined` | Already defined twice in `main.c` — known issue; suppressed by compiler warning, not a build failure |

---

## 4. Flash and Monitor

```bash
# Replace /dev/ttyUSB0 with the actual serial port
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor

# Combined flash + monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

Find the serial port:

```bash
ls /dev/tty* | grep -E 'USB|ACM|usbmodem'
```

Exit the monitor with **Ctrl+]**.

### 4.1 Erasing NVS (odometer / stored config)

The odometer persists across reboots in NVS. To reset it for a clean test:

```bash
idf.py -p /dev/ttyUSB0 erase-flash
# Then re-flash
idf.py -p /dev/ttyUSB0 flash
```

Or erase only the NVS partition (safer, preserves firmware):

```bash
esptool.py --chip esp32p4 -p /dev/ttyUSB0 erase_region 0x9000 0x6000
```

The NVS partition starts at `0x9000` and is `0x6000` bytes per `partitions.csv`.

---

## 5. Key Configuration Knobs ("Feature Flags")

There are no feature-flag services in this project. All configuration is done
via **C preprocessor defines** in `main/main.c` and **Kconfig** in `sdkconfig`.

### 5.1 Sensor source (most important)

**File:** `main/main.c`, line ~37

```c
#define SENSOR_SOURCE SENSOR_SOURCE_ANALOG   // change to SENSOR_SOURCE_CAN for CAN
```

| Value | Effect |
|-------|--------|
| `SENSOR_SOURCE_ANALOG` | (default) ADC sensors + GPS + tach interrupt |
| `SENSOR_SOURCE_CAN` | CAN bus; auto-detects bitrate and protocol; disables analog ADC/tach tasks |

This single define rewires the entire task graph at boot (`app_main`). Changing
it requires a rebuild and re-flash.

### 5.2 Enable verbose ESP-IDF logs

**File:** `main/main.c`, line ~85

```c
#define ENABLE_LOGS false    // set to true to emit ESP_LOGx output
```

Enabling this floods the monitor with temperature, pressure, AFR, GPS, and fuel
readings — useful for sensor calibration.

### 5.3 Lap timer track location

**File:** `main/lap_timer/lap_timer.c`, lines 13–14

```c
#define FINISH_LAT 47.254635
#define FINISH_LON -123.191149
```

Hardcoded to Ridge Motorsports Park. Update these for a different track.

### 5.4 Odometer compiled-in default

**File:** `main/odometer/odometer.c`, line 24

```c
static uint64_t total_meters = 3701491;   // compiled default
```

Used only when NVS contains no valid record. Represents ~2300 miles at
compile time. Override before first flash on a new board.

### 5.5 Kconfig / sdkconfig

Run `idf.py menuconfig` to adjust chip, flash, PSRAM, LVGL, and FreeRTOS
options. `sdkconfig.defaults` carries the opinionated defaults; after any
change to it, rebuild from scratch (`idf.py fullclean && idf.py build`).

---

## 6. Codebase Areas and Testing Workflows

### 6.1 UI / LVGL (`main/tach_ui/`)

**Entry point:** `main/tach_ui/ui.c` → `ui_init()`; boot screen in
`main/tach_ui/boot/boot_screen.c`.

**Manual test:** Flash and observe the display. The LVGL performance monitor
(`CONFIG_LV_USE_PERF_MONITOR=y` in sdkconfig) renders FPS and CPU load on
screen — check this after every layout change.

**Changing the tach image for a 4in screen:**
Replace `main/tach_ui/images/ui_img_1656279599.c` with the 720×720 version
from the Google Drive link in the README, rebuild, and reflash.

**Verify UI after a change:**
1. Build and flash.
2. Monitor: confirm `bsp_display_brightness_set(50)` log appears (backlight on).
3. Look at the screen — gauges should animate within a second of boot.

---

### 6.2 Sensor Reading (Analog Mode — `main/main.c`)

Analog mode runs three FreeRTOS tasks: `tach_task`, `gps_task`, `adc_task`.

**To verify sensor math without hardware:**
1. Set `ENABLE_LOGS true` in `main/main.c`.
2. Build, flash, open monitor.
3. Grounding or floating ADC pins should produce predictable raw ADC values
   (0 or 4095). Use these to verify the conversion math in `read_temp_resistance`,
   `voltage_to_psi`, `fuel_pct_from_voltage`, etc.
4. Cross-check against the lookup tables (`sensorR[]`, `tempF[]`, `Vpts[]`,
   `PSIpts[]`) with a hand calculation.

**RPM tach ISR:** Triggered by a rising edge on GPIO 5. To simulate without
an ignition signal, toggle GPIO 5 from an external MCU or a bench function
generator at the expected frequency (`RPM = 60,000,000 / (periodUs × 2)`).

---

### 6.3 CAN Bus (`main/canbus/`)

**Switch to CAN mode** (see §5.1), rebuild, and flash.

**Protocols:** JSON files under `main/canbus/protocols/`:
- `haltech.json` — 1 Mbit/s, IDs 0x360–0x3E0
- `hondata.json`
- `link_g4x.json`
- `maxxecu.json`
- `ecumasters_black.json`

Protocol JSON is embedded at build time via `protocol_list.h` (generated from
the JSON files by `main/canbus/CMakeLists.txt` or a `xxd`/`incbin` step —
verify with `idf.py build` output if changed). Do not rename or add JSON files
without updating the protocol list include.

**Auto-detection:** On boot, `canbus_init()` tries bitrates 1M → 500k → 250k →
125k, waiting for ≥3 standard frames at each rate. Then `protocol_detect()`
identifies the protocol from frame IDs (≥2 hits required).

**Adding a new protocol:**
1. Create `main/canbus/protocols/myecu.json` following the existing schema.
2. Add it to `protocol_list.h` (or whichever mechanism embeds it).
3. Map new signal names in `signal_name_to_ptr()` in `protocol_loader.c` if
   the signal targets a new `can_dash_data_t` field.
4. Update `canbus.h` (`can_dash_data_t` struct) accordingly.

**Testing CAN without a real ECU:**
Use a secondary ESP32 or any CAN-capable MCU to replay recorded frames at the
target bitrate. The transceiver wiring is: TX → GPIO 5, RX → GPIO 4.

---

### 6.4 GPS (`main/gps_wrap/`)

GPS UART: `UART_NUM_3`, RX on GPIO 35, 115200 baud.

`gps_task` feeds raw NMEA bytes into `gps_encode_char()` (TinyGPS++ wrapper).
Speed, fix status, lat/lon, satellite count, and HDOP are exposed via
`gps_get_speed_mph()`, `gps_has_fix()`, etc.

**Testing without GPS hardware:**
1. Set `ENABLE_LOGS true`.
2. No GPS signal → `has_fix` will remain `false`; speed label shows `--`.
3. To inject NMEA sentences, wire a USB-Serial adapter to GPIO 35 and use
   `minicom` or `screen` to send a GPRMC sentence at 115200 baud.

---

### 6.5 Odometer (`main/odometer/`)

Persisted in NVS using a dual-slot CRC-verified scheme. Saved every 100 meters
of accumulated travel (`SAVE_INTERVAL_M`), plus a periodic 3-second flush from
`save_miles_task`.

**To reset odometer on a live board (without full flash):**
Erase NVS partition (see §4.1). On next boot, the compiled default
`total_meters` is used.

**To set a specific mileage:** Edit `total_meters` default in `odometer.c`,
erase NVS, rebuild, and reflash.

---

### 6.6 Lap Timer (`main/lap_timer/`)

Hardcoded to Ridge Motorsports Park. Triggered when GPS coordinates are within
10 m of the finish line at speed ≥10 mph, debounced at 10 s.

**Testing:** Update `FINISH_LAT`/`FINISH_LON` to match your location, rebuild,
drive/walk past the point. Monitor output (`ENABLE_LOGS true`) will show
`LAP_TIMER` events. Predictive delta is streamed over UART with the gauge
payload.

---

### 6.7 UART Gauge Payload (`main/main.c` — `adc_task`/`can_mapping_task`)

Both sensor modes transmit a 26-byte packet every 20 ms on **two UARTs**
simultaneously (UART1 at GPIO 37, UART2 at GPIO 30), at 2 Mbit/s.

Packet structure (`gauge_payload_t`): oil_temp, water_temp, oil_pressure,
fuel_pressure, fuel_level, AFR, boost_psi, lap_time_ms, lap_delta_ms, +
SOF byte, sequence byte, CRC16-CCITT.

**Testing:** Connect a USB-Serial adapter to GPIO 37 or GPIO 30 at 2 Mbit/s
and capture raw bytes. Decode against the struct layout. The `GAUGE_PKT_SOF`
byte (`0xA5`) marks every packet boundary.

---

## 7. Partition Table Reference

`partitions.csv`:

| Name | Type | SubType | Offset | Size |
|------|------|---------|--------|------|
| nvs | data | nvs | 0x9000 | 0x6000 |
| phy_init | data | phy | 0xF000 | 0x1000 |
| factory | app | factory | 0x10000 | 8 MB |
| storage | data | spiffs | (after factory) | 7 MB |

The `storage` SPIFFS partition is mounted at `/spiffs` by `mount_fs()` in CAN
mode. It holds CAN protocol data. In analog mode it is never mounted.

---

## 8. Common Gotchas

- **ADC2 conflicts with Wi-Fi.** This project does not use Wi-Fi, but if you
  add it, ADC2 channels (oil temp, boost, fuel, AFR) will stop working.
- **`ADC_ATTEN` defined twice** in `main.c` (lines ~116 and ~140). The second
  definition shadows the first; both resolve to `ADC_ATTEN_DB_11`. Not a bug,
  but will generate a compiler warning.
- **Dual UART at 2 Mbit/s.** ESP32-P4 UART clock supports this, but some USB-
  Serial adapters do not. Use a FTDI FT232H or similar high-speed adapter.
- **LVGL arc widget range:** `lv_arc_set_value(ui_rpm_arc, rpm)` expects an
  integer 0–9000. The arc range is set in `ui_Screen1.c` during `ui_init()`;
  keep `MAX_RPM` in sync with that range.
- **NVS version conflicts:** If you change the NVS schema, erase NVS before
  flashing to avoid `ESP_ERR_NVS_NEW_VERSION_FOUND` on boot.

---

## 9. Updating This Skill

Update this file whenever you:

- Discover a new build error and its fix.
- Add or change a compile-time configuration knob.
- Add a new CAN protocol and learn how to test it.
- Identify a new hardware testing trick (e.g., simulating a sensor).
- Encounter a partition layout or IDF version mismatch.
- Find that any instruction in this file is wrong or stale.

**Commit format:** Include `[runbook]` in the commit message, e.g.:
`[runbook] Document how to simulate GPS NMEA injection`

Keep entries concrete: exact file paths, line numbers, commands, and expected
output. Avoid vague advice.
