---
name: cloud-agent-starter
description: Use when starting work in this ESP-IDF repository and needing immediate run, test, and workflow setup instructions for Cloud agents.
---

# Cloud Agent Starter (ESP32 P4 Gauge Cluster)

Minimal runbook for Cloud agents working in this repo.

## 0) First 2 minutes

1. Start in repo root: `/workspace`
2. Load ESP-IDF env:
   - `source "$HOME/esp/esp-idf/export.sh"`
3. Sanity-check toolchain:
   - `idf.py --version`
   - `idf.py set-target esp32p4`
4. Fast compile check:
   - `idf.py build`

If `idf.py` is missing, stop and document the exact failure in your PR/testing notes.

## 1) Common Cloud workflow commands

- Build only:
  - `idf.py build`
- Full clean + rebuild:
  - `idf.py fullclean build`
- Flash firmware:
  - `idf.py -p /dev/ttyACM0 flash`
- Serial monitor:
  - `idf.py -p /dev/ttyACM0 monitor`
- Flash + monitor:
  - `idf.py -p /dev/ttyACM0 flash monitor`

If serial device differs, list likely ports first:
- `ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null`

## 2) Codebase areas and practical test workflows

### A) UI + app orchestration (`main/main.c`, `main/tach_ui/*`)

Use when touching display logic, labels, color/state transitions, task startup.

Workflow:
1. Build: `idf.py build`
2. Flash + monitor: `idf.py -p <PORT> flash monitor`
3. Verify no boot-time crash and UI boots.
4. Validate touched UI element behavior by inspecting on-device output.

Smoke checks from monitor logs:
- No panic/reset loop
- Tasks start cleanly from `app_main`

### B) Sensor path selection + feature toggle pattern (`main/main.c`)

The repo uses a compile-time switch, not remote feature flags:
- `#define SENSOR_SOURCE SENSOR_SOURCE_ANALOG`
- `#define SENSOR_SOURCE SENSOR_SOURCE_CAN`

Workflow:
1. Edit `SENSOR_SOURCE` in `main/main.c`.
2. Rebuild + flash.
3. Confirm expected task set starts:
   - CAN mode: `canbus_task` + `can_mapping_task`
   - Analog mode: `tach_task` + `gps_task` + `adc_task`

Tip: treat this as the project’s primary "feature flag" for data source behavior.

### C) CAN stack + protocol mapping (`main/canbus/*`, `main/canbus/protocols/*.json`)

Use when changing CAN parsing, detection, scaling, protocol JSON.

Workflow:
1. Build: `idf.py build`
2. Ensure protocol JSON embeds by successful build (generated `protocol_list.c` path is wired by `main/CMakeLists.txt`).
3. Flash in CAN mode (`SENSOR_SOURCE_CAN`).
4. Monitor and verify:
   - bitrate detection logs
   - protocol detection logs
   - stable updates to mapped values (RPM/speed/coolant/oil pressure)

If no real CAN source is available, still run build + boot and record what could/could not be validated.

### D) GPS / lap timer / odometer (`main/gps_wrap/*`, `main/lap_timer/*`, `main/odometer/*`)

Use when touching location, lap timing, persistence, or distance accumulation.

Workflow:
1. Build + flash in analog mode.
2. Monitor for GPS fix behavior and non-fix fallback (`--` speed label behavior).
3. Confirm odometer init/save flow does not error (NVS init succeeds).
4. For lap timer changes, verify no regressions in elapsed/delta updates.

If live GPS is unavailable in Cloud, compile and boot-test only, and document limitation clearly.

## 3) "Login" and credentials notes for Cloud agents

This firmware repo has no app-layer auth/login flow. "Login" typically means toolchain/device access:

- Ensure shell has ESP-IDF env loaded via `export.sh`.
- Ensure serial device access exists for flashing/monitoring.
- If external services/tools are added later, document auth steps here in this skill.

## 4) Fast pre-PR verification checklist

Run this minimum set for firmware changes:

1. `source "$HOME/esp/esp-idf/export.sh"`
2. `idf.py set-target esp32p4`
3. `idf.py build`
4. If hardware available: `idf.py -p <PORT> flash monitor` and validate touched area.

Always include:
- Exact commands run
- Pass/fail status
- Any environment limits (no board, no CAN feed, no GPS fix)

## 5) Keeping this skill updated (short runbook hygiene)

When you discover a new reliable testing trick or recovery step:

1. Add it to the relevant area section above (A/B/C/D), not a random notes dump.
2. Keep additions concrete: command + when to use + expected signal.
3. Remove stale steps in the same PR if they are no longer correct.
4. In PR summary, include one line: "Updated `skills/cloud-agent-starter/SKILL.md` with <new trick>."

Rule of thumb: optimize for the next Cloud agent’s first 10 minutes.
