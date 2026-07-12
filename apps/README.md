# apps/

Bench-test tooling for the center cluster. **Not part of the firmware build** —
a standalone helper for exercising the cluster on the bench. Its signal map is
generated from the firmware's real CAN decoder (`main/canbus.c`,
`main/protocols/link_g4x.json`), so the math matches what the cluster
actually does.

| App | What it is | How to run |
|---|---|---|
| `trackcluster-can-sender/` | Unified desktop app (pywebview + python-can) with a **device selector** — transmits either the Center Cluster (0x3E8–0x3EE) or RealDash (0x3EF–0x3F1) frame set to a connected CAN-USB adapter. Auto-detects adapters, selectable bitrate, Send Once / Send Continuously. Fully self-contained / portable. | See `trackcluster-can-sender/BUILD.md` — run from source or build a portable `.exe`. |
