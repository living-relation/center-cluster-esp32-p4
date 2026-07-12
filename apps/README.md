# apps/

Bench-test tooling for the center cluster. **Not part of the firmware build** —
these are standalone helpers for exercising the cluster on the bench. Both are
generated from the firmware's real CAN decoder (`main/canbus.c`,
`main/protocols/link_g4x.json`), so the signal math matches what the cluster
actually does.

| App | What it is | How to run |
|---|---|---|
| `canbus-bench-test.html` | Browser-only tool. Pick a gauge, type the value it should show, copy the computed 8-byte CAN frame into CANgaroo / PCAN-View / SavvyCAN. No hardware access. | Double-click the file — opens in any browser. Works on desktop and mobile. |
| `trackcluster-can-sender/` | Unified desktop app (pywebview + python-can) with a **device selector** — transmits either the Center Cluster (0x3E8–0x3EE) or RealDash (0x3EF–0x3F1) frame set to a connected CAN-USB adapter. Auto-detects adapters, selectable bitrate, Send Once / Send Continuously. | See `trackcluster-can-sender/BUILD.md` — run from source or build a portable `.exe`. |

The tools deliberately keep **separate copies** of the signal map;
`trackcluster-can-sender` never modifies `canbus-bench-test.html`.
