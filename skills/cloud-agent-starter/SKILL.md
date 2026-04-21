---
name: cloud-agent-starter
description: Use when starting work in this ESP-IDF gauge-cluster repo and needing immediate environment setup, build/flash commands, UI preview testing, and per-area test workflows for Cloud agents.
---

# Cloud agent starter (ESP32 P4 gauge cluster)

Minimal runbook so a Cloud agent can build, sanity-check, and test without hunting through the tree.

## Environment and “login”

There is no application login or API keys in this firmware. “Getting in” means having a working ESP-IDF toolchain and (optional) USB serial access to a board.

1. Work from repo root: `/workspace`.
2. Prefer the repo helper (sources IDF and runs `idf.py`):
   - `./dev.sh --build-only` — compile only, no device.
   - `./dev.sh` — auto-pick serial port and `flash monitor` (fails clearly if no port).
   - `./dev.sh --port /dev/ttyACM0` — explicit port.
3. The helper defaults to `IDF_HELPER=/home/ubuntu/.esp/use-idf54.sh`. If that file is missing, set `IDF_HELPER` to a script that sources your ESP-IDF `export.sh`, or source IDF yourself, then run `idf.py` directly.

If `idf.py` is missing after sourcing, stop and record the exact error in PR notes.

## “Feature flags” and toggles

There are no remote feature flags. Use these instead:

| What you need | Where / how |
|-----------------|-------------|
| Analog sensors vs CAN data path | `main/main.c`: `SENSOR_SOURCE` — `SENSOR_SOURCE_ANALOG` vs `SENSOR_SOURCE_CAN`. Change, rebuild, reflash. |
| UI themes (preview only) | `tools/ui_preview`: pass `sport`, `oem`, or `stealth` to the preview binary or use the gallery script (see UI preview area). |
| Kconfig / sdkconfig | `idf.py menuconfig` or edit `sdkconfig` / `sdkconfig.defaults` for ESP-IDF options; document any non-default choices in the PR. |

## One-command “start the app” (firmware)

- Build only: `./dev.sh --build-only`
- Flash + serial monitor (hardware required): `./dev.sh` or `./dev.sh --port <device>`

Raw equivalents after IDF is sourced: `idf.py set-target esp32p4`, `idf.py build`, `idf.py -p <PORT> flash monitor`.

List candidate serial devices: `ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null`

## Codebase areas and concrete test workflows

### A) Firmware app + UI on device (`main/main.c`, `main/tach_ui/*`, `main/lv_conf.h`)

**When:** display code, boot flow, tasks, LVGL wiring, gauge behavior on hardware.

**Workflow:**

1. `./dev.sh --build-only`
2. If a board is attached: `./dev.sh` (or `--port …`) and watch the monitor.
3. Confirm: no panic/reboot loop, UI reaches a steady state, touched widgets behave as expected.

**Signals:** clean boot logs from `app_main`, no repeated Guru Meditation / assert traces.

### B) Sensor source “mode” (`main/main.c` — `SENSOR_SOURCE`)

**When:** CAN vs analog wiring, which tasks should run.

**Workflow:**

1. Set `#define SENSOR_SOURCE` to `SENSOR_SOURCE_CAN` or `SENSOR_SOURCE_ANALOG`.
2. `./dev.sh --build-only`; then flash if hardware is available.
3. In monitor, confirm the expected task set for that mode (e.g. CAN-related work only when CAN is selected).

**Cloud without CAN bus:** still run build (and boot if you have a board); state that live CAN validation was not possible.

### C) CAN stack and protocols (`main/canbus/*`, `main/canbus/protocols/*.json`)

**When:** parsing, detection, protocol JSON, scaling.

**Workflow:**

1. `./dev.sh --build-only` — ensures protocol assets embed and link (see `main/CMakeLists.txt` for generation).
2. Build and run with `SENSOR_SOURCE_CAN` when exercising CAN paths.
3. On hardware with a CAN source: flash + monitor; check detection/bitrate logs and mapped fields (RPM, speed, temps, pressure).

**Mock without hardware:** use the UI preview tool (area E) for static gauge values, not full bus simulation.

### D) GPS, lap timer, odometer (`main/gps_wrap/*`, `main/lap_timer/*`, `main/odometer/*`)

**When:** GPS UART, lap timing, NVS persistence, distance.

**Workflow:**

1. Use analog mode (`SENSOR_SOURCE_ANALOG`) if that is how GPS tasks are enabled in your branch.
2. `./dev.sh --build-only`; flash + monitor if you have GPS + board.
3. Check: NVS init succeeds, no crash when no fix, lap/odometer logic does not spam errors.

**Cloud without sky view:** compile + boot-only is acceptable; say “no GPS fix available” in verification notes.

### E) LVGL UI preview — no flash (`tools/ui_preview/*`)

**When:** layout/themes, `ui_Screen1`, visual regression without ESP32.

**Workflow (recommended):**

1. From repo root: `tools/ui_preview/generate_gallery.sh`  
   - For headless/CI: `tools/ui_preview/generate_gallery.sh --no-open`
2. Confirm artifacts exist under `tools/ui_preview/output/` (`theme_*.png`, `index.html`).
3. The preview injects fixed “CAN-like” demo values (see `tools/ui_preview/README.md` — e.g. RPM, MPH, gear, odometer).

**Dependencies:** host `gcc`/`g++`, `cmake`, ImageMagick `convert` on PATH.

### F) BSP and bundled components (`components/bsp_extra/*`, `components/TinyGPSPlus-ESPIDF/*`)

**When:** board support, display init, extra drivers.

**Workflow:**

1. `./dev.sh --build-only` after any change under `components/`.
2. Flash + monitor if the change affects runtime hardware init.

## Fast pre-PR checklist

1. `./dev.sh --build-only` (or `idf.py build` with IDF sourced).
2. For UI-only visual changes: `tools/ui_preview/generate_gallery.sh --no-open`.
3. If hardware is available: `./dev.sh` and note what was observed.
4. In the PR description, list: commands run, pass/fail, and limits (no board / no CAN / no GPS).

## Updating this skill

When you learn a new reproducible step (CI image path, new script, sdkconfig gotcha, preview dependency):

1. Add it under the matching **codebase area** above with a **command + when to use + what “good” looks like**.
2. Remove or correct outdated steps in the same change so the next agent does not hit contradictions.
3. In the PR summary, add one line: `Updated skills/cloud-agent-starter/SKILL.md: <short reason>.`

Aim for what the next Cloud agent needs in the **first 10 minutes**.
