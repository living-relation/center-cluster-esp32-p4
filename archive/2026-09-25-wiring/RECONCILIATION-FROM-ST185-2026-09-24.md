# Cluster wiring reconciliation — handed over from the ST185 repo

Written 2026-09-24 by the ST185 harness session. **Nothing here has been fixed.**
This is a hand-off so a center-cluster session can do the work with the right
sources in front of it.

Two things happened:

1. A cross-repo audit (2026-09-22) found three cluster conflicts, one CRITICAL.
   They are internal to this repo and were never the ST185 session's to fix.
2. `st185-link-ecu-config` was carrying a **stale duplicate** of this repo's
   cluster GPIO / J8 / encoder / UART content. Daniel's instruction: the ECU↔CAN
   interface stays in ST185, everything cluster-internal comes back here. The
   stale copy has been **deleted** from ST185 rather than moved, because this
   repo's version is newer — but what it said is recorded below so you know what
   was circulating.

Authority for cluster GPIO: `main/Kconfig.projbuild` + `PINOUT.md` outrank any
copy elsewhere.

---

## C1 — CRITICAL: J8 physical pin numbers disagree inside this repo

Same GPIO lands on different Phoenix / J8 pins depending on which file you read.

| Net | `PINOUT.md` (silk-derived) | `docs/harness/HARNESS_WIRING_DIAGRAM.html` + `CONNECTOR_BOM_AND_HARNESS.md` |
|---|---|---|
| CAN TX GPIO5 | pin **16** | **T11** |
| CAN RX GPIO4 | pin **18** | **T12** |
| UART1 TX GPIO20 | pin **13** | **T22** |
| UART2 TX GPIO21 | pin **11** | **T13** |
| ODO GPIO29 | pin **7** | **T18** |
| ENC1 A GPIO30 | pin **40** | **T19** |
| ENC1 B GPIO31 | pin **36** | **T21** |
| ENC2 A GPIO49 | pin **26** | **T32** |
| ENC2 B GPIO50 | pin **23** | **T34** |
| ENC2 SW GPIO51 | pin **29** | **T36** |
| Pin 4 | **5V** | **GND** |
| Pin 23 | **GPIO50** | **GPIO28 (labelled free)** |

Also clashes on pins 12, 19, 21, 22, 24, 32, 34, 36.

**Why it matters:** building to the harness diagram lands CAN, UART and both
encoders on the wrong header pins. Pin 4 disagreeing between 5V and GND is the
dangerous one.

**Not in dispute:** the GPIO numbers themselves. `WIRING.md`, `PINOUT.md` and
`Kconfig.projbuild` agree on CAN 4/5, UART TX 20/21, ODO 29, Enc1 30/31/32,
Enc2 49/50/51. The fight is only over which J8 pin each one is.

---

## C2 — HIGH: firmware has controls the harness docs do not

| Claim | Firmware + `PINOUT.md` | Harness diagram / CONNECTOR_BOM |
|---|---|---|
| Encoder 3, backlight dim | GPIO **24 / 25 / 2** | missing entirely |
| Headlight sense | GPIO **28**, active-low, wired | marked **T23 GPIO28 (free)** |
| Inter-cluster UART | **TX-only** (GPIO20, GPIO21), no center RX | — |

---

## C3 — HIGH: `Kconfig.projbuild` contradicts itself on GPIO28

The comment around L46–49 says GPIO22 and GPIO28 "are therefore free on J8".
`TC_HEADLIGHT_GPIO` defaults to **28**. Runtime uses 28; the comment is wrong.

---

## What the ST185 repo was carrying, and what it got wrong

`st185-link-ecu-config/WIRING.md` §1, §3, §4, §5, §6 held a copy of this repo's
cluster wiring. Deleted from there 2026-09-24. Its errors, for the record:

| Item | Stale ST185 copy said | This repo says |
|---|---|---|
| Inter-cluster UART RX | GPIO **18** / **19** reserved as center RX | TX-only; no center RX |
| Encoder 3 | absent | GPIO 24 / 25 / 2 |
| Headlight sense | absent | GPIO 28, active-low |

`st185-link-ecu-config/apps/harness-schematic/index.html` still lists
`"GPIO18 UART1 RX"` and `"GPIO19 UART2 RX"` — same stale reservation. That file
is an ST185 artifact and will be corrected there; flagged here only so the two
repos end up telling the same story.

---

## What legitimately stays in ST185

The ECU-facing CAN interface, which originated there and is not cluster-internal:

- ECU ↔ center CAN wiring and bus topology
- The 5-node shared bus (ECU, CSB3 switchboard, center cluster, RealDash Pi)
- `CAN-BUS-ID-ALLOCATION-TABLE.md` — message content and IDs

The center cluster is a **node on that bus**. Its internal GPIO, J8 pinout,
encoders, buttons and inter-cluster UART are this repo's.

---

## Suggested order for whoever picks this up

1. Settle C1 first — pick one of `PINOUT.md` or the harness diagram as the J8
   authority and make the other match. Check pin 4 (5V vs GND) against the
   physical board before trusting either.
2. Fix C3 (one comment) while you are in `Kconfig.projbuild`.
3. Add Encoder 3 and headlight sense to the harness diagram and
   `CONNECTOR_BOM_AND_HARNESS.md` (C2).
4. Re-check `TrackCluster — Center Cluster Assembly Harness` on harness.design
   (document `69xl`, last touched 2026-09-13) — it predates Encoder 3 and
   headlight sense, so it is stale too.

Source audit: `harness-ecu-wiring-audit.md`, 2026-09-22, findings E1–E3.
