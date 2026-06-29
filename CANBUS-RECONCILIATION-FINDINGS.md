<!-- STATUS: APPLIED 2026-06-28 — (1) MAP-not-MGP fix and (2) oil+fuel pressure widened to 2 bytes (0x3E9: oil=bytes3-4, fuel=bytes5-6) are now in canbus.c, main/protocols/link_g4x.json, link_g4x_can_setup.json, and CANBUS-LINK-G4X-CONFIG.md. REQUIRES A REFLASH of the center cluster. Matching ECU + RealDash config updated in the st185-link-ecu-config repo. -->
# CAN Bus Check: Link G4X ↔ Center Cluster

**What this is:** I researched how a Link G4X ECU is supposed to send custom CAN data, then
checked your center cluster's CAN setup against those rules. This is the result, in plain words.

---

## Bottom line

- Most of your setup is **correct**. The math the dashboard uses to undo the ECU's numbers
  matches the Link rules.
- **3 things need fixing.** One is a wrong parameter name (boost will read wrong). Two are
  "size too small" problems where real pressure values won't fit and get cut off.
- A few smaller things should be **double-checked on the ECU** before you trust it.

---

## The Link G4X rule (simple version)

On a Link G4X you build your own CAN messages. For each value you send, you set six things:

1. **Start position** – which byte it starts at.
2. **Width (size)** – how many bytes (8 bits = 1 byte, 16 bits = 2 bytes).
3. **Byte order** – "MS" means biggest part first (big-endian). Your setup uses this. Good.
4. **Type** – signed (can go negative) or unsigned (zero and up only).
5. **Multiplier / Divider** – stretches or shrinks the number.
6. **Offset** – shifts the number up or down.

**The formula the ECU uses to make the number on the wire:**

> wire number = (real value × Multiplier ÷ Divider) + Offset

**The dashboard must do the exact reverse to get the real value back:**

> real value = (wire number − Offset) × Divider ÷ Multiplier

One important G4X detail: the G4X sends the **real, on-screen value** (like 90 °C, or 250 kPa),
not a hidden internal number. (Older G4+ ECUs did it differently — don't copy old G4+ guides.)

In your files, "scale" = Multiplier (with Divider = 1), and "offset" = Offset.

---

## Channel-by-channel check

Sizes and offsets are what you asked me to watch closest. Here's every channel.

| Frame | Byte(s) | Channel | Size | Offset | Scale | Dashboard math | Verdict |
|---|---|---|---|---|---|---|---|
| 0x3E8 | 0–1 | Engine RPM | 2 | 0 | ×1 | reads raw rpm | ✅ Correct |
| 0x3E8 | 2–3 | **MAP / boost** | 2 | 0 | ×1 | (value − 100) → boost | ⚠️ **Wrong param name** |
| 0x3E8 | 4 | Coolant (ECT) | 1 | +50 | ×1 | value − 50, then °C→°F | ✅ Correct |
| 0x3E8 | 5 | Intake air (IAT) | 1 | +50 | ×1 | value − 50, then °C→°F | ✅ Correct |
| 0x3E8 | 6 | Oil temp | 1 | +50 | ×1 | value − 50, then °C→°F | ✅ Correct |
| 0x3E9 | 0–1 | Ignition angle | 2 | +1000 | ×10 | (value ÷10) − 100 | ✅ Correct |
| 0x3E9 | 2 | Vehicle speed | 1 | 0 | ×1 | km/h → mph | ⚠️ Size tight |
| 0x3E9 | 3 | **Oil pressure** | 1 | 0 | ×1 | kPa → psi | ❌ **Size too small** |
| 0x3E9 | 4 | **Fuel pressure** | 1 | 0 | ×1 | kPa → psi | ❌ **Size too small** |
| 0x3EA | 0–1 | Lambda | 2 | 0 | ×1000 | value ÷ 1000 | ✅ Correct |
| 0x3EB | 0 | Gear | 1 | 0 | ×1 | 7 → reverse | ✅ Correct (verify) |
| 0x3EB | 1 | Fuel level | 1 | 0 | ×1 | reads % | ✅ Correct (calibrate) |
| 0x3EE | 0–5 | 6 warning flags | 1 each | 0 | ×1 | 0 = OK, else alarm | ✅ Correct (verify) |

---

## The 3 problems

### 1. Boost: you named the wrong ECU parameter (HIGH priority)

- Your config file calls this channel **"MGP"**, but the dashboard expects **MAP**. These are
  not the same thing:
  - **MAP** = absolute pressure. Idle ≈ 100. Boost is anything over 100. Never negative.
  - **MGP** = gauge pressure. Idle ≈ 0. Vacuum is negative. Boost is positive.
- The dashboard does `boost = value − 100`. That's only right if the ECU sends **MAP**.
- If you send **MGP** (like the name says), at idle the dash would compute `0 − 100 = −100 kPa`
  and show about **−14.5 psi of boost** all the time. Wrong.
- **Fix:** on the ECU, assign **MAP** (manifold *absolute* pressure) to bytes 2–3, and fix the
  name in the config file. No firmware change needed.

### 2. Oil pressure won't fit in 1 byte (HIGH priority)

- 1 byte can only hold 0–255. You send raw kPa, so the most you can ever send is **255 kPa = 37 psi**.
- But your own dashboard redline for oil pressure is **125 psi** (`DASH_OIL_PRESS_REDLINE`).
- So anything above 37 psi gets cut off — the gauge would freeze near the bottom under real
  oil pressure. This is the "size" problem you wanted me to watch for.
- **Fix:** make oil pressure **2 bytes** (holds up to 65,535). Re-layout the frame and update
  the firmware (exact changes below).

### 3. Fuel pressure won't fit in 1 byte (HIGH priority)

- Same problem. Max you can send is **37 psi**, but your fuel gauge goes to **160 psi**
  (`DASH_FUEL_PRESS_DISPLAY_MAX`).
- **Fix:** make fuel pressure **2 bytes** too.

### Smaller note — vehicle speed (LOW priority)

- 1 byte km/h maxes out at **255 km/h ≈ 158 mph**. Fine for most cars. If this car can go
  faster, make speed 2 bytes as well.

---

## How to fix (copy-paste ready)

The boost fix (#1) is just an ECU change. The pressure fixes (#2, #3) change the byte layout of
frame **0x3E9**, so the ECU config, the firmware, and the JSON map all change together.

### New layout for frame 0x3E9 (speed stays 1 byte)

```
 byte:  0    1    2      3    4       5    6        7
       [ ign  ] [spd] [ oil press ] [ fuel press ] [--]
        u16     u8      u16            u16          spare
```

### A) Firmware — `main/canbus.c`, function `decode_3e9`

Replace the current body with:

```c
static void decode_3e9(const uint8_t *d)
{
    float ign = (float)be_u16(d + 0) * 0.1f - 100.0f;

    portENTER_CRITICAL(&g_dash_mux);
    g_dash.ign_adv    = ign;
    g_dash.mph        = d[2] * 0.621371f;
    g_dash.oil_press  = be_u16(d + 3) * 0.145038f;   /* was d[3] — now 2 bytes */
    g_dash.fuel_press = be_u16(d + 5) * 0.145038f;   /* was d[4] — now 2 bytes */
    portEXIT_CRITICAL(&g_dash_mux);
}
```

Also update the comment block above it so bytes read: `[0–1] ign  [2] speed  [3–4] oil  [5–6] fuel  [7] spare`.

### B) ECU config map — `main/protocols/link_g4x.json`, frame `0x3E9`

```json
{
  "id": "0x3E9", "period_ms": 10,
  "channels": [
    {"name": "ign_angle",     "byte": 0, "len": 2, "type": "u16", "scale": 0.1,      "offset_val": -100, "unit": "deg"},
    {"name": "speed",         "byte": 2, "len": 1, "type": "u8",  "scale": 0.621371, "offset_val": 0,    "unit": "mph"},
    {"name": "oil_pressure",  "byte": 3, "len": 2, "type": "u16", "scale": 0.145038, "offset_val": 0,    "unit": "psi"},
    {"name": "fuel_pressure", "byte": 5, "len": 2, "type": "u16", "scale": 0.145038, "offset_val": 0,    "unit": "psi"}
  ]
}
```

### C) Source-of-truth map — `link_g4x_can_setup.json`

- In frame **0x3E8**, change the MAP channel's `pclink_name` from `"MGP"` to `"MAP"` and the
  note to "absolute manifold pressure."
- In frame **0x3E9**, set oil pressure to `byte 3, length 2` and fuel pressure to `byte 5, length 2`.

### D) On the ECU (PCLink), in CAN → your transmit streams

- **0x3E8 bytes 2–3:** assign **MAP** (absolute), Width 16, MS, Unsigned, Mult 1, Div 1, Offset 0.
- **0x3E9 oil pressure:** Start byte 3, **Width 16**, MS, Unsigned, Mult 1, Div 1, Offset 0.
- **0x3E9 fuel pressure:** Start byte 5, **Width 16**, MS, Unsigned, Mult 1, Div 1, Offset 0.

---

## Things to verify on the ECU (no code change, just confirm)

1. **Display units feeding CAN.** Confirm the ECU is sending temps in **°C**, pressures in **kPa**,
   speed in **km/h**, lambda as **lambda** (not AFR). Use PCLink's **CAN test calculator** to
   check the wire number matches what the dashboard expects.
2. **Gear numbers.** The dashboard expects `0 = neutral, 1–6 = gears, 7 = reverse`. Confirm the
   Link "Gear Position" parameter actually outputs those exact numbers.
3. **Warning flags (0x3EE).** Link sends limit/protection info as a group of bits. Make sure each
   of the 6 bytes is wired to the right Link flag (knock, ign cut, fuel cut, boost cut, sensor,
   throttle). This is the fuzziest part of the setup.

---

## Quick test after changes

1. Build the firmware. (CI workflow `esp-idf-build.yml` already exists.)
2. Use the built-in injector to fake a high oil pressure and confirm it shows correctly:
   - `can 3e9 09C40032 04B0 0000` style — send a 2-byte oil value like `0x0258` (600 kPa ≈ 87 psi)
     and confirm the left display reads ~87 psi, not a clipped number.
3. On the car: idle the engine and confirm **boost reads near 0 psi** (proves the MAP fix worked).

---

## Sources

- [Link CAN Setup (official support)](https://support.linkecu.com/hc/en-us/articles/1500002420301-CAN-Setup)
- [Link G4+ Manual — CAN Setup Examples (multiplier/divider/offset formula)](https://www.akao.co.uk/manuals/Link%20G4+%20Manual/can_setup_examples.htm)
- [Link forum — User defined CAN stream settings (G4X uses displayed value)](https://forums.linkecu.com/topic/11701-user-defined-can-stream-settings/)
- [HPAcademy — MGP vs MAP vs BAP](https://www.hpacademy.com/forum/general-tuning-discussion/show/mgp-vs-map-vs-bap/)
- Repo files reviewed: `link_g4x_can_setup.json`, `main/protocols/link_g4x.json`, `main/canbus.c`, `main/dash_data.h`, `CANBUS-LINK-G4X-CONFIG.md`
