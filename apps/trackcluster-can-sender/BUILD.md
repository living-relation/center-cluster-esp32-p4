# TrackCluster CAN Live Sender — unified (RealDash + Center Cluster)

One bench app that transmits either device's CAN stream. Pick the device in the top
selector; the correct signal map + descriptions load instantly. The two signal maps are
**byte-for-byte copies** of the two original per-device apps — no CAN ID, byte layout,
scaling, or endianness was changed. This app only merges the launcher/back end.

- **RealDash** → sends `0x3EF / 0x3F0 / 0x3F1` (matches `link_g4x_realdash.xml`, the RD-layout
  binding source).
- **Center Cluster** → sends `0x3E8 / 0x3E9 / 0x3EA / 0x3EB / 0x3EE` (matches ESP32-P4
  `main/canbus.c`).

Both verified against the committed GitHub configs (the flashed state) — no discrepancies.

## Launch
- **Desktop shortcut "TrackCluster CAN Sender"** → runs the portable exe. Pick device in-app.
- From source (dev): `pythonw.exe app.py` using the shared venv at
  `..\canbus-live-sender\.venv`. Deep-link a device for testing with env `TC_DEVICE=cluster`
  or `TC_DEVICE=realdash`.

## Back end (shared by both profiles)
Auto-detects + hot-plug auto-connects (connect-only; never transmits until you press Send /
enable continuous). Unplug detection. Bitrate selector (default 1 Mbit/s — both devices).
Adapters: gs_usb/candleLight (bundled libusb), slcan, seeedstudio, and pcan/kvaser/vector when
that vendor driver is installed. Switching device does NOT drop the adapter link; it does stop
any running continuous sends (safety).

## Portable build (single folder, any Windows PC)
Run from this folder, using the shared venv's Python (has deps + pyinstaller):
```powershell
$py = "..\canbus-live-sender\.venv\Scripts\python.exe"
& $py -m PyInstaller --noconfirm --clean --windowed ^
  --name "TrackCluster CAN Sender" --icon app.ico --add-data "ui;ui" ^
  --collect-all libusb_package --collect-all webview --collect-all clr_loader ^
  --collect-all pythonnet --collect-submodules can app.py
```
Output: `dist\TrackCluster CAN Sender\` (~30 MB) — copy the whole folder anywhere and run the
`.exe`. Needs WebView2 (preinstalled Win10 22H2+/11). Older candleLight firmware may still need a
one-time Zadig → WinUSB bind for gs_usb.

## Notes
- The two original per-device apps remain untouched in their repos as the authoritative sources.
- `dist/`, `build/`, `.venv/`, `*.spec` are gitignored.
