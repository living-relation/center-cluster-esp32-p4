# apps/

Bench-test tooling for the center cluster. **Not part of the firmware build** —
these are standalone helpers for exercising the cluster on the bench. Both are
generated from the firmware's real CAN decoder (`main/canbus.c`,
`main/protocols/link_g4x.json`), so the signal math matches what the cluster
actually does.

| App | What it is | How to run |
|---|---|---|
| `canbus-bench-test.html` | Browser-only tool. Pick a gauge, type the value it should show, copy the computed 8-byte CAN frame into CANgaroo / PCAN-View / SavvyCAN. No hardware access. | Double-click the file — opens in any browser. Works on desktop and mobile. |
| `canbus-live-sender/` | Desktop app (pywebview + python-can) with the same UI, but it actually transmits to a connected CAN-USB adapter (candleLight/gs_usb or slcan), including Send Once / Send Continuously. | See `canbus-live-sender/BUILD.md` — run from source or build a standalone `.exe`. |

The two tools deliberately keep **separate copies** of the signal map;
`canbus-live-sender` never modifies `canbus-bench-test.html`.
