<!-- Created 2026-09-25 · feature/center-cluster-harness-design · Cowork: center-cluster doc cleanup + archive -->

# Archived 2026-09-25 — do not build from

- **Harness docs** (`HARNESS_WIRING_DIAGRAM.html`, `CONNECTOR_BOM_AND_HARNESS.md`,
  `CONNECTOR_BOM.csv`, `wiring_diagram.svg`, `trackcluster-center-harness.v0.9.json`):
  wrong J8 pin (T#) numbers on every signal, pin 4 shown as GND (it is 5V), no
  Encoder 3 or headlight sense, and a screw-terminal adapter (SM-A-001) that was
  never used. Superseded.
- **`RECONCILIATION-FROM-ST185-2026-09-24.md`**: the hand-off that flagged the
  above. Resolved by this archive; kept for history only.

Current sources: J8 pinout → `PINOUT.md` (backed by the board silkscreen).
Parts to buy → `docs/harness/PURCHASE-LIST.csv`. Assembly drawing → harness.design
(TrackCluster Assembly, pending).
