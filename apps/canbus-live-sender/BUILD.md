# CAN Live Sender — build & run

Separate desktop app, companion to `apps/canbus-bench-test.html` (which stays browser-only
and unmodified). This one actually transmits to a CAN-USB adapter via `python-can`.

## Run from source (fastest way to iterate)

```powershell
cd tools\canbus-live-sender
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

A native window opens with the same UI as the bench-test tool, plus a Connect panel at the top.

## One-time adapter setup (Windows)

- **candleLight/gs_usb firmware** (the default on this hardware, same as CANgaroo uses): the
  device needs the **WinUSB** driver bound via **[Zadig](https://zadig.akeo.ie/)** — same
  one-time step CANgaroo already required. Run Zadig, select the CANable device, install WinUSB,
  then relaunch this app.
- **slcan firmware**: shows up as a normal COM port, no driver step needed.

## Package as a standalone `.exe`

```powershell
pip install pyinstaller
pyinstaller --onefile --windowed --add-data "ui;ui" --name CANLiveSender app.py
```

The finished executable is at `dist\CANLiveSender.exe` — copy that file anywhere and double-click
to run; no Python install needed on the target machine.

Notes:
- `--add-data "ui;ui"` bundles the `ui/index.html` file into the executable (Windows uses `;` as
  the separator; it's `:` on macOS/Linux if this is ever built there).
- pywebview uses the Edge **WebView2** runtime on Windows, which ships pre-installed on
  Windows 10 (22H2+) and Windows 11. If it's somehow missing, install the
  [WebView2 Evergreen Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) once.
- The `.exe` is unsigned, so Windows SmartScreen will likely warn on first run — click
  "More info" → "Run anyway".

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| "No adapters found" | Adapter not plugged in, or (gs_usb) Zadig driver not bound yet |
| Connect succeeds but Send fails immediately | Adapter unplugged mid-session, or another app (CANgaroo, SavvyCAN) already has it open — close the other app first |
| Gauges don't react on the cluster | Confirm CAN wiring (`apps/canbus-bench-test.html` bus-settings panel), and that "Send continuously" is checked for the frames you care about |
