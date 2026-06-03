<!-- STATUS: CODED-COMPLETE — config artifacts shipped: link_g4x_can_setup.lcs / .json (this folder); firmware decode map main/protocols/link_g4x.json -->
# Link G4X — PCLink CAN setup (ECU side)

Apply this to the **Link G4X FuryX** in PCLink so it broadcasts exactly the frames the center
cluster decodes. The matching config is shipped two ways in this folder:

- **`link_g4x_can_setup.lcs`** — import directly: PCLink → **CAN → Setup → File → Open**.
- **`link_g4x_can_setup.json`** — human-readable canonical copy of the same map.

The firmware's decoder (`main/protocols/link_g4x.json`) is a strict match of this; don't let
them drift. **Assign only the channels below** — extra channels just waste bus bandwidth.

---

## Verified system topology (this repo)

This CAN setup assumes the exact hardware topology used by the center-cluster firmware:

- **Center cluster (ESP32-P4)** is the **only** board on ECU CAN.
- Center TWAI pins come from `sdkconfig` / `Kconfig.projbuild`:
   - `CONFIG_TC_CAN_TX_GPIO=5` (center GPIO5 -> transceiver TXD)
   - `CONFIG_TC_CAN_RX_GPIO=4` (center GPIO4 <- transceiver RXD)
- **Left and right displays are not on CAN**. They receive data from center over UART:
   - Center `UART1 TX GPIO20` -> Left board `GPIO44 RX`
   - Center `UART2 TX GPIO21` -> Right board `GPIO44 RX`

So the ECU must transmit only to the center transceiver path; side displays follow whatever
the center forwards in its UART bridge frames.

---

## Global CAN settings (PCLink → CAN → Custom Setup)

| Setting | Value |
|---|---|
| CAN module | the module wired to the dashboard transceiver |
| Bit rate | **1 Mbit/s** |
| Stream type | **Custom** |
| Byte order | **Big endian** |
| Transmit | streams 0x3E8–0x3EB and 0x3EE (below) |
| Receive (optional) | 0x3EC / 0x3ED — only if you want center encoder selections to change ECU boost-map / traction |

---

## Transmit streams (ECU → dashboard)

### 0x3E8 — engine fast (10 ms)
| Byte | Len | Channel | Scale | Offset | On wire |
|---|---|---|---|---|---|
| 0 | 2 | Engine Speed (RPM) | ×1 | 0 | RPM |
| 2 | 2 | MAP (absolute) | ×1 | 0 | kPa abs (firmware −100 → boost) |
| 4 | 1 | ECT (coolant) | ×1 | +50 | °C+50 |
| 5 | 1 | IAT | ×1 | +50 | °C+50 |
| 6 | 1 | Oil Temp | ×1 | +50 | °C+50 |

### 0x3E9 — speed / pressure / ignition (10 ms)
| Byte | Len | Channel | Scale | Offset | On wire |
|---|---|---|---|---|---|
| 0 | 2 | Ignition Angle | ×10 | +1000 | 0.1°+100° |
| 2 | 1 | Vehicle Speed | ×1 | 0 | km/h (firmware → MPH) |
| 3 | 1 | Oil Pressure | ×1 | 0 | kPa (firmware → PSI) |
| 4 | 1 | Fuel Pressure | ×1 | 0 | kPa (firmware → PSI) |

### 0x3EA — lambda (10 ms)
| Byte | Len | Channel | Scale | Offset | On wire |
|---|---|---|---|---|---|
| 0 | 2 | Lambda 1 | ×1000 | 0 | 0.001 λ |

### 0x3EB — gear / fuel (50 ms)
| Byte | Len | Channel | Scale | Offset | On wire |
|---|---|---|---|---|---|
| 0 | 1 | Gear Position | ×1 | 0 | 0=N, 1-6, 7=R (firmware 7→−1) |
| 1 | 1 | Fuel Level | ×1 | 0 | % (calibrate the AnVolt tank sender 0–100% first) |

### 0x3EE — engine protection (50 ms)  ← drives the right-screen warning overlay
One byte per alarm: **0 = OK, nonzero = active**. Map each byte to the matching Link limit /
protection flag (Link broadcasts limit flags as a bit field — route the relevant bit/level to
each byte, or send the limit % directly).

| Byte | Len | Meaning | Suggested Link source |
|---|---|---|---|
| 0 | 1 | Knock | Knock Level / Knock Limit active |
| 1 | 1 | Ignition cut | Ignition Cut % (limiter active) |
| 2 | 1 | Fuel cut | Fuel Cut % (limiter active) |
| 3 | 1 | Boost cut | Boost Limit / Overboost active |
| 4 | 1 | Sensor error | any sensor / fault code active |
| 5 | 1 | Throttle error | TPS / ETB error |

> Do **not** put gauge-shown conditions (oil pressure, fuel level, coolant/oil/IAT temps) in
> 0x3EE — those are already communicated by gauge color on the screens. 0x3EE is only for the
> full-screen "ECU WARNING" overlay.

---

## Adding / removing a channel later

The order matters — never broadcast a channel the firmware has no field for:
1. Add the field to `main/dash_data.h` (`dash_data_t`).
2. Decode it in `main/canbus.c`; carry it in the relevant UART frame in `main/dash_data.c`
   (free bytes are reserved) and render it in the cluster's `ui_*.c`.
3. **Then** assign it on the next free byte of a stream here, and update
   `main/protocols/link_g4x.json` + this file's `.lcs`/`.json` to match.

To remove a sensor, strip it from the UI/data model and from PCLink in the same change.

---

## Validate (PCLink side)
1. CAN → Monitor: confirm 0x3E8–0x3EB and 0x3EE appear at their cycle times.
2. Cross-check each value against the live runtime page (should match within rounding).
3. Power the dashboard; gauges should track within ~2 s, and an engine-protection flag should
   raise the right-cluster warning overlay.

## Validate (full vehicle wiring)
1. Confirm CAN transceiver is wired only to the center board (GPIO5 TX, GPIO4 RX) and ECU CANH/CANL.
2. Confirm side displays have UART wiring from center GPIO20/21 TX lines to each side GPIO44 RX, plus common GND.
3. With ECU live, center should update from CAN and both side displays should mirror updates over UART even though they have no direct CAN connection.
