# TrackCluster Connector BOM and Harness Plan

Purpose: connector-by-connector wiring list with buyable part numbers for housings and crimp contacts.

This is a production-oriented harness plan for the center P4 + left/right S3 topology already defined in WIRING.md.

## 0) Important build assumption

The center board J8 is an unshrouded 2x20, 2.54 mm header. There is no sealed automotive mating shell for it.
Use a small interposer board that plugs into J8, then bring all vehicle wiring out through sealed connectors.

Recommended J8 interposer mate (board-to-board, no crimp):

| Item | Part number | Notes |
|---|---|---|
| 2x20 female socket strip, 2.54 mm | Samtec SSW-120-02-G-D | Mates to center J8 male header |
| Alternate | Sullins PPPC202LFBN-RC | Equivalent 2x20 female socket strip |

## 1) Connector-by-connector list (sealed vehicle harness side)

Connector family below is Deutsch DT, widely used in automotive environments.

Common crimp terminals used by all DT connectors in this table:

| Contact type | Part number | Wire range |
|---|---|---|
| Pin (male contact) | Deutsch 0460-202-16141 | 20-16 AWG |
| Socket (female contact) | Deutsch 0462-201-16141 | 20-16 AWG |

## 2) Harness connectors and pin maps

### C1: ECU CAN trunk to center transceiver

| Field | Value |
|---|---|
| Connector pair | DT04-3P (receptacle) + DT06-3S (plug) |
| Wedgelocks | W3P + W3S |
| Conductors | CANH, CANL, GND drain/reference |
| Suggested wire | 22 AWG twisted pair for CANH/CANL + 22 AWG for GND |

Pin map:

| C1 pin | Net | Destination |
|---|---|---|
| 1 | CANH | CAN transceiver CANH |
| 2 | CANL | CAN transceiver CANL |
| 3 | GND | Transceiver/bus reference GND |

### C2: Center interposer to left display serial link

| Field | Value |
|---|---|
| Connector pair | DT04-3P + DT06-3S |
| Wedgelocks | W3P + W3S |
| Conductors | TX_L, RX_L (reserved), GND |
| Suggested wire | 24 AWG twisted pair (TX/RX) + 24 AWG GND |

Pin map:

| C2 pin | Net | Center side | Left side |
|---|---|---|---|
| 1 | TX_L | GPIO20 (UART1 TX) | GPIO44 (UART RX) |
| 2 | RX_L (optional) | GPIO18 (UART1 RX) | GPIO43 (UART TX) |
| 3 | GND | GND | GND |

### C3: Center interposer to right display serial link

| Field | Value |
|---|---|
| Connector pair | DT04-3P + DT06-3S |
| Wedgelocks | W3P + W3S |
| Conductors | TX_R, RX_R (reserved), GND |
| Suggested wire | 24 AWG twisted pair (TX/RX) + 24 AWG GND |

Pin map:

| C3 pin | Net | Center side | Right side |
|---|---|---|---|
| 1 | TX_R | GPIO21 (UART2 TX) | GPIO44 (UART RX) |
| 2 | RX_R (optional) | GPIO19 (UART2 RX) | GPIO43 (UART TX) |
| 3 | GND | GND | GND |

### C4: 5V power feed to each display node

Use one connector per board so each module can be disconnected independently.

| Field | Value |
|---|---|
| Connector pair | DT04-2P + DT06-2S |
| Wedgelocks | W2P + W2S |
| Conductors | +5V, GND |
| Suggested wire | 20 AWG (or 18 AWG for long runs) |

Pin map:

| C4 pin | Net | Destination |
|---|---|---|
| 1 | +5V | Board VIN/5V input |
| 2 | GND | Board GND |

## 3) Board-local pigtails (inside enclosure)

Because J8 is not a sealed automotive connector, terminate to short pigtails on the interposer PCB:

| Interposer signal group | Recommended board connector | Notes |
|---|---|---|
| CAN logic (TXD/RXD/3V3/GND) | JST GH 1.25 mm, 4-pin: BM04B-GHS-TBT | Mating housing: GHR-04V-S, contacts: SSHL-002T-P0.2 |
| Left UART (TX/RX/GND) | JST GH 1.25 mm, 3-pin: BM03B-GHS-TBT | Mating housing: GHR-03V-S, contacts: SSHL-002T-P0.2 |
| Right UART (TX/RX/GND) | JST GH 1.25 mm, 3-pin: BM03B-GHS-TBT | Mating housing: GHR-03V-S, contacts: SSHL-002T-P0.2 |
| 5V input to center interposer | JST VH 3.96 mm, 2-pin: B2B-VH | Mating housing: VHR-2N, contacts: SVH-21T-P1.1 |

## 4) Center GPIO to connector mapping (authoritative)

These values mirror sdkconfig and Kconfig.projbuild:

| Net | Center GPIO |
|---|---:|
| CAN_TXD | 5 |
| CAN_RXD | 4 |
| UART1_TX_LEFT | 20 |
| UART1_RX_LEFT | 18 |
| UART2_TX_RIGHT | 21 |
| UART2_RX_RIGHT | 19 |
| ODO_BUTTON | 29 |
| ENC1_A/B/SW | 30 / 31 / 32 |
| ENC2_A/B/SW | 49 / 50 / 51 |

## 5) Purchasing checklist (single vehicle set)

| Qty | Part number | Use |
|---:|---|---|
| 3 | DT04-3P | CAN + two UART receptacles |
| 3 | DT06-3S | CAN + two UART plugs |
| 3 | W3P | DT 3-way wedgelocks |
| 3 | W3S | DT 3-way wedgelocks |
| 3 | DT04-2P | 5V feeds receptacles |
| 3 | DT06-2S | 5V feeds plugs |
| 3 | W2P | DT 2-way wedgelocks |
| 3 | W2S | DT 2-way wedgelocks |
| 30 | 0460-202-16141 | DT pin contacts (male) |
| 30 | 0462-201-16141 | DT socket contacts (female) |
| 1 | SSW-120-02-G-D | Center J8 interposer socket |
| 2 | BM03B-GHS-TBT | UART board-local headers |
| 1 | BM04B-GHS-TBT | CAN board-local header |
| 1 | B2B-VH | Interposer 5V board-local header |
| 6 | GHR-03V-S | 3-pin GH housings |
| 2 | GHR-04V-S | 4-pin GH housings |
| 4 | VHR-2N | 2-pin VH housings |
| 40 | SSHL-002T-P0.2 | GH crimp contacts |
| 10 | SVH-21T-P1.1 | VH crimp contacts |

## 6) Notes before crimping

- Keep CAN pair twisted from ECU branch to transceiver.
- Keep UART TX/RX as twisted pair with ground reference nearby.
- Star-ground all three displays and transceiver back to buck output ground.
- Side displays should be flashed over USB-C; do not share UART console on GPIO43/44 during runtime.
- If your board revision uses a different side-board connector pitch, keep the DT harness unchanged and only swap the board-local pigtail connector family.

## 7) Verify before ordering

Because Waveshare sometimes changes connector footprints between revisions:

1. Confirm center J8 is standard 2x20, 2.54 mm, 0.64 mm square posts.
2. Confirm side-board UART connector pitch/pin order on your exact PCB revision.
3. If any board-side connector differs, keep the net mapping above and substitute only that local connector family.
