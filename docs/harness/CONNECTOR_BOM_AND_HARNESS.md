# TrackCluster Connector BOM and Harness Plan

Purpose: connector-by-connector wiring list with buyable part numbers for housings and crimp contacts.

This is a production-oriented harness plan for the center P4 + left/right S3 topology already defined in WIRING.md.

## 0) Important build assumption

The center board J8 is an unshrouded 2x20, 2.54 mm header. There is no sealed automotive mating shell for it.
Plug a **40-pin screw-terminal GPIO adapter** onto J8 so harness wires land on Phoenix-style clamps (bare wire or ferrules), then route vehicle Deutsch DT leads from the center enclosure.

Recommended J8 screw-terminal adapter (2.54 mm, 40 pos):

| Item | Part number | Buy |
|---|---|---|
| Screw-mount breakout (primary) | Sequent SM-A-001 / B-RPI-X1-SM-RT | [sequentmicrosystems.com/products/breakout-card-screw-mount-for-raspberry-pi](https://sequentmicrosystems.com/products/breakout-card-screw-mount-for-raspberry-pi) |
| Alternate labeled board | DIYables screw terminal expansion | [diyables.io/products/screw-terminal-block-expansion-board-for-raspberry-pi](https://diyables.io/products/screw-terminal-block-expansion-board-for-raspberry-pi) |
| Replacement 2-pos terminal (optional) | Phoenix MKDS 1,5/ 2-3.5 (1775578) | [phoenixcontact.com — MKDS 1,5/ 2-3.5](https://www.phoenixcontact.com/en-us/products/screw-terminal-block-mkds-15-2-35-1775578) |

Terminal **T01–T40** on the adapter = J8 physical pins **1–40**. See `HARNESS_WIRING_DIAGRAM.html` for the block diagram, full pinout, and shopping list.

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

### C2: Center J8 adapter to left display serial link

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
| 2 | RX_L (optional) | GPIO28 (UART1 RX) | GPIO43 (UART TX) |
| 3 | GND | GND | GND |

### C3: Center J8 adapter to right display serial link

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
| 2 | RX_R (optional) | GPIO22 (UART2 RX) | GPIO43 (UART TX) |
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

## 3) Board-local wiring (inside center enclosure)

From the J8 screw-terminal adapter, run short leads to the SN65HVD230 CAN board and to Deutsch DT pigtails for the vehicle harness:

| Signal group | Center terminal | Destination |
|---|---|---|
| CAN logic | T01 (3V3), T39 (GND), T11 (CTX), T12 (CRX) | SN65HVD230 board pads |
| CAN bus | Transceiver CANH/CANL | C1 pins 1–2 |
| Left UART | T22 (TX), T23 (RX), T39 (GND) | C2 |
| Right UART | T13 (TX), T24 (RX), T39 (GND) | C3 |
| 5V in | T02 (+5V), T39 (GND) | C4 center feed + buck return |
| Controls | T18, T19, T21, T31, T32, T34, T36 + GND | C5–C7 |

## 4) Center GPIO → Phoenix terminal mapping (authoritative)

GPIO numbers mirror sdkconfig / Kconfig.projbuild. **T#** = J8 physical pin = adapter screw terminal.

| Net | GPIO | Terminal T# |
|---|---:|---:|
| +5V in | 5V rail | 2 |
| GND ref | GND | 39 |
| CAN_TXD | 5 | 11 |
| CAN_RXD | 4 | 12 |
| UART1_TX_LEFT | 20 | 22 |
| UART1_RX_LEFT | 18 | 23 |
| UART2_TX_RIGHT | 21 | 13 |
| UART2_RX_RIGHT | 19 | 24 |
| ODO_BUTTON | 29 | 18 |
| ENC1_A / B / SW | 30 / 31 / 32 | 19 / 21 / 31 |
| ENC2_A / B / SW | 49 / 50 / 51 | 32 / 34 / 36 |
| CAN board VCC | 3V3 | 1 |

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
| 1 | SM-A-001 | J8 40-pos screw-terminal adapter |
| 1 | SN65HVD230 CAN Board | Center CAN transceiver ([Waveshare](https://www.waveshare.com/sn65hvd230-can-board.htm)) |
| 1 | D36V50F5 | 12 V→5 V buck ([Pololu #4091](https://www.pololu.com/product/4091)) |

Full shopping list with links: open `HARNESS_WIRING_DIAGRAM.html` in a browser (print landscape 11×17).

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
