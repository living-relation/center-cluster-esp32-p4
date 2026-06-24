# AGENTS.md

## Cursor Cloud specific instructions

This repo is **ESP-IDF firmware** for the center instrument cluster on a Waveshare
**ESP32-P4** board (800×800 MIPI-DSI display). The "application" is firmware that gets
flashed to that board — there is **no host-runnable binary or web UI**. In the cloud VM
(no board, no display) the available dev loop is **configure → build → flashable image**;
`flash`/`monitor` require the physical board over USB.

### Toolchain (already installed in the VM image)
- ESP-IDF **v5.4.2** lives at `~/esp/esp-idf`; tools/python venv under `~/.espressif`.
  Pinned at 5.4.x on purpose: 5.3.x lacks stable P4 MIPI-DSI; 5.5+ dropped early
  ESP32-P4 v1.x silicon support (see `README.md` / `SETUP_BEFORE_YOU_BUILD.txt`).
- **Source the env in every new shell before any `idf.py` command:**
  `. ~/esp/esp-idf/export.sh`  (the startup update script does not persist exports).

### Build / lint / test (run from repo root, after sourcing export.sh)
- Target is auto-selected from `sdkconfig.defaults` (`CONFIG_IDF_TARGET="esp32p4"`); to set
  it explicitly use `idf.py set-target esp32p4`.
- Build: `idf.py build` → produces `build/center_cluster.bin` (+ bootloader at **0x2000**,
  required for P4 boot; verify with `grep 0x2000 build/flash_args`).
- Footprint: `idf.py size`. There is **no automated test suite** and no separate linter in
  this repo; the ESP-IDF GCC build (with `-Werror`-style warnings) is the correctness gate,
  matching CI (`.github/workflows/esp-idf-build.yml`, which builds with esp-idf v5.4 / esp32p4).

### Config notes (non-obvious)
- `sdkconfig` is **git-ignored** and regenerated from `sdkconfig.defaults`; deleting it and
  rebuilding restores the shipping defaults (live mode, PSRAM Hex 200 MHz, 16 MB flash).
- Ships with **bench mode OFF** (live CAN/UART). To compile the standalone demo-sweep code
  path without hardware, set `CONFIG_TC_BENCH_MODE=y` (via `idf.py menuconfig` → TrackCluster,
  or appended to `sdkconfig`) and rebuild. Revert by regenerating `sdkconfig`.
- `managed_components/` (LVGL, esp_lvgl_port, esp_lcd_jd9365, es8311) are fetched from the
  component registry on first configure/build per `main/idf_component.yml` / `dependencies.lock`.
- The PowerShell helpers in `scripts/` are **Windows-only** flashing aids (need a COM port);
  they are not usable in the Linux cloud VM. Build directly with `idf.py build` instead.
