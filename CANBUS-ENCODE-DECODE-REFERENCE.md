<!-- Canonical CAN cross-reference for the TrackCluster center cluster (ESP32-P4)
     paired with a Link G4X XtremeX ECU. Agent-readable: hand this to the ECU-side
     config agent so ENCODE (ECU transmit) and DECODE (cluster firmware) never drift.
     Derived from main/canbus.c + main/protocols/link_g4x.json + link_g4x_can_setup.json. -->
# CAN encode / decode reference — XtremeX ↔ center cluster

Every channel, both directions, in one place so the **ECU transmit** setup and the
**cluster decode** are never confused. The two sides use **inverse** scale/offset:

- **ENCODE** = what the **Link G4X XtremeX** does when building the frame (PCLink → CAN →
  Custom Setup). Link formula: `wire = round(value × Multiplier ÷ Divider) + Offset`.
- **DECODE** = what the **cluster firmware** (`main/canbus.c`) does to recover the value.

> ⚠️ **Do not copy the DECODE scale/offset into PCLink.** The ECU multiplier is the
> reciprocal of the cluster scale, and the ECU offset is the negative of the cluster offset.
> Example: lambda is **×1000** on the ECU and **×0.001** in the cluster — same relationship,
> opposite direction.

## Bus / topology

| Item | Value |
|---|---|
| ECU | Link G4X **XtremeX** |
| Dashboard CAN bus | **1 Mbit/s**, **big-endian (MS byte first)**, standard **11-bit** IDs, 8-byte frames |
| Only node the ECU talks to | center cluster (ESP32-P4) — TWAI TX=GPIO5 / RX=GPIO4 via SN65HVD230 |
| Side displays | not on CAN — center forwards data over UART |
| **Lambda** | no onboard wideband: an **external Link CAN Lambda controller** sits on the ECU's **CAN1** bus; the XtremeX receives it into the **Lambda 1** parameter and rebroadcasts it to the cluster on `0x3EA` (below) |

## Master table (ECU → cluster broadcast)

`u16` = 2 bytes big-endian, `u8` = 1 byte. Offsets/scales are the exact PCLink and firmware values.

| Frame | Cycle | Byte | Size | Channel | PCLink parameter | ENCODE (ECU): Mult, Offset → wire | DECODE (cluster): formula → unit | Example (value → wire → value) |
|---|---|---|---|---|---|---|---|---|
| `0x3E8` | 10 ms | 0 | u16 | Engine RPM | Engine Speed | ×1, +0 → RPM | `wire × 1` → RPM | 3500 → `0x0DAC` → 3500 RPM |
| `0x3E8` | 10 ms | 2 | u16 | MAP absolute | MAP (**not MGP**) | ×1, +0 → kPa abs | `(wire − 100) × 0.145038` → **boost PSI** | 200 kPa → `0x00C8` → 14.5 psi |
| `0x3E8` | 10 ms | 4 | u8 | Coolant (ECT) | ECT | ×1, **+50** → °C+50 | `(wire − 50) × 9/5 + 32` → °F | 90 °C → `0x8C` → 194 °F |
| `0x3E8` | 10 ms | 5 | u8 | Intake air (IAT) | IAT | ×1, **+50** → °C+50 | `(wire − 50) × 9/5 + 32` → °F | 30 °C → `0x50` → 86 °F |
| `0x3E8` | 10 ms | 6 | u8 | Oil temp | Oil Temp | ×1, **+50** → °C+50 | `(wire − 50) × 9/5 + 32` → °F | 100 °C → `0x96` → 212 °F |
| `0x3E8` | 10 ms | 7 | — | (unassigned) | — | leave empty | — | — |
| `0x3E9` | 10 ms | 0 | u16 | Ignition angle | Ign Angle | **×10, +1000** → 0.1°+100° | `wire × 0.1 − 100` → °BTDC | 25° → `0x04E2` → 25° |
| `0x3E9` | 10 ms | 2 | u8 | Vehicle speed | Vehicle Speed | ×1, +0 → km/h | `wire × 0.621371` → mph | 100 km/h → `0x64` → 62 mph |
| `0x3E9` | 10 ms | 3 | u16 | Oil pressure | Oil Pressure | ×1, +0 → kPa | `wire × 0.145038` → PSI | 450 kPa → `0x01C2` → 65 psi |
| `0x3E9` | 10 ms | 5 | u16 | Fuel pressure | Fuel Pressure | ×1, +0 → kPa | `wire × 0.145038` → PSI | 300 kPa → `0x012C` → 44 psi |
| `0x3E9` | 10 ms | 7 | — | (unassigned) | — | leave empty | — | — |
| `0x3EA` | 10 ms | 0 | u16 | Lambda 1 | Lambda 1 (via CAN1 Link CAN Lambda) | **×1000, +0** → 0.001 λ | `wire × 0.001` → λ | 0.90 λ → `0x0384` → 0.90 λ |
| `0x3EB` | 50 ms | 0 | u8 | Gear position | Gear Position | ×1, +0 → int | `7 → R`, else `wire` → gear | 3 → `0x03` → 3;  R → `0x07` → R |
| `0x3EB` | 50 ms | 1 | u8 | Fuel level | An Volt x (calibrated 0–100 %) | ×1, +0 → % | `wire` → % | 75 % → `0x4B` → 75 % |
| `0x3EE` | 50 ms | 0 | u8 | Knock | Knock Level / Limit | 0 = OK, nonzero = active | nonzero → warn bit | active → `0x01` → overlay |
| `0x3EE` | 50 ms | 1 | u8 | Ignition cut | Ignition Cut % | 0 = OK, nonzero = active | nonzero → warn bit | — |
| `0x3EE` | 50 ms | 2 | u8 | Fuel cut | Fuel Cut % | 0 = OK, nonzero = active | nonzero → warn bit | — |
| `0x3EE` | 50 ms | 3 | u8 | Boost cut | Boost Limit / Overboost | 0 = OK, nonzero = active | nonzero → warn bit | — |
| `0x3EE` | 50 ms | 4 | u8 | Sensor error | Fault Code Active | 0 = OK, nonzero = active | nonzero → warn bit | — |
| `0x3EE` | 50 ms | 5 | u8 | Throttle error | TPS / ETB Error | 0 = OK, nonzero = active | nonzero → warn bit | — |

**`0x3EE` rules:** one byte per alarm, `0 = OK / nonzero = active`. Do **not** put gauge-shown
conditions (oil pressure, fuel level, coolant/oil/IAT temps) here — those are already shown by
gauge color. This frame only drives the right-cluster full-screen "ECU WARNING" overlay
(priority high→low: knock → ign-cut → fuel-cut → boost-cut → sensor → throttle).

## Cluster → ECU (the ECU must RECEIVE these)

The center cluster transmits a 1-byte selection frame on a button press (never on a timer):

| Frame | Size | Payload | Meaning |
|---|---|---|---|
| `0x3EC` | u8 | map index `0–3` | Boost-map selection |
| `0x3ED` | u8 | angle index `0–4` | Traction slip-angle selection |

## Round-trip identities (sanity checks)

```
RPM:        cluster = wire × 1                         ECU wire = RPM
MAP→boost:  boost_psi = (wire − 100) × 0.145038        ECU wire = MAP_kPa_abs
Temps:      °F = (wire − 50) × 9/5 + 32                ECU wire = °C + 50
Ignition:   ° = wire × 0.1 − 100                       ECU wire = ° × 10 + 1000
Speed:      mph = wire × 0.621371                      ECU wire = km/h
Pressure:   psi = wire × 0.145038                      ECU wire = kPa
Lambda:     λ = wire × 0.001                           ECU wire = λ × 1000
```

*Source of truth in this repo: `main/canbus.c` (decode) and `link_g4x_can_setup.json` (ECU
transmit). Keep this table, those two files, `main/protocols/link_g4x.json`, and
`CANBUS-LINK-G4X-CONFIG.md` in lockstep — change all of them in the same edit.*
