/**
 * dash_data.h — TrackCluster master dash data model + UART bridge protocol.
 *
 * This file is the SHARED, CANONICAL header. All three firmware projects
 * (center-cluster-esp32-p4, left-side-cluster-esp32s3, right-side-cluster-esp32s3)
 * include this exact header — the per-cluster `center-dash_data.h` /
 * `left-dash_data.h` / `right-dash_data.h` files are thin shims that #include this.
 *
 *  - Center cluster:  PRODUCER. Decodes ECU CAN, populates the struct, transmits the
 *                      per-side UART frames.
 *  - Left cluster:    UART-only consumer. Receives LEFT UART frame (FRAME_TYPE 0x01)
 *                      from center. No sensor signals reach it directly.
 *  - Right cluster:   UART-only consumer. Receives RIGHT UART frame (FRAME_TYPE 0x02)
 *                      from center. No sensor signals reach it directly.
 *
 * Every field is rendered by at least one display. The ECU CAN stream is configured
 * in PCLink to broadcast ONLY the channels the dashboard renders — no extra channels
 * are present on the bus. The firmware decodes every frame it receives.
 */

#ifndef TRACKCLUSTER_DASH_DATA_H
#define TRACKCLUSTER_DASH_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Status flag bits (only flags driven by displayed channels) ────────────── */
#define DASH_FLAG_OIL_PRESS_LOW  (1u << 0)
#define DASH_FLAG_FUEL_LOW       (1u << 1)
#define DASH_FLAG_SHIFT          (1u << 2)
#define DASH_FLAG_COOLANT_HOT    (1u << 4)
#define DASH_FLAG_OIL_TEMP_HOT   (1u << 5)
#define DASH_FLAG_LAMBDA_BAD     (1u << 6)
#define DASH_FLAG_OVERBOOST      (1u << 7)

/* ── Engine-protection warning bits (decoded from ECU status frame 0x3EE) ────
 * These drive the RIGHT cluster full-screen warning overlay (yellow bg / red box /
 * black text). Listed HIGH → LOW priority; the overlay shows up to 3 active at once.
 * NOTE: widget-represented conditions (low oil pressure, low fuel, high temps) are
 * NOT here — they are already shown by color on their gauges. */
#define DASH_WARN_KNOCK      (1u << 0)
#define DASH_WARN_IGN_CUT    (1u << 1)
#define DASH_WARN_FUEL_CUT   (1u << 2)
#define DASH_WARN_BOOST_CUT  (1u << 3)
#define DASH_WARN_SENSOR     (1u << 4)
#define DASH_WARN_THROTTLE   (1u << 5)

/* ── Odometer mode enum ────────────────────────────────────────────────────── */
typedef enum {
    DASH_ODO    = 0,
    DASH_TRIP_A = 1,
    DASH_TRIP_B = 2,
} dash_odo_mode_t;

/* ── Master struct — only fields rendered on a display ─────────────────────── */
typedef struct {
    /* Engine */
    float    rpm;           /* center: tach arc, shift LEDs, gear-box tint   */
    float    lambda;        /* right:  digital readout                       */
    float    boost;         /* right:  bar gauge (derived from MAP)          */
    float    ign_adv;       /* right:  bar gauge                             */

    /* Temperatures (°F) */
    float    oil_temp;      /* left:   mini arc                              */
    float    coolant_temp;  /* right:  bar gauge                             */
    float    iat;           /* right:  bar gauge                             */

    /* Pressures (PSI) */
    float    oil_press;     /* left:   mini arc                              */
    float    fuel_press;    /* left:   mini arc                              */

    /* Driveline */
    float    mph;           /* left:   speedo arc + digital                  */
    int8_t   gear;          /* center: gear glyph                            */

    /* Fuel */
    float    fuel_level;    /* left:   mini arc                              */

    /* Trip / odo — computed locally on center by integrating mph */
    float    odo;
    float    trip_a;
    float    trip_b;
    uint8_t  odo_mode;      /* dash_odo_mode_t                               */

    /* Status & freshness */
    uint16_t flags;
    uint8_t  warn;          /* engine-protection warnings (DASH_WARN_* bits) */
    uint32_t last_update_ms;

    /* Input menu state — set by center, forwarded in the RIGHT UART frame.
     * The right display renders a popup overlay when menu_id != 0. */
    uint8_t  menu_id;      /* 0=hidden, 1=BOOST MAP, 2=TC SLIP ANGLE */
    uint8_t  menu_cursor;  /* currently highlighted item index (0-based) */
} dash_data_t;

/* Global snapshot. Mutated under a critical section in the producer (center). */
extern volatile dash_data_t g_dash;

/* ── UART bridge protocol ─────────────────────────────────────────────────── */
#define UART_BRIDGE_BAUD         921600u
#define UART_BRIDGE_FRAME_LEN    32u
#define UART_BRIDGE_SOF          0xA5u
#define UART_BRIDGE_EOF          0x5Au
#define UART_BRIDGE_TYPE_LEFT    0x01u
#define UART_BRIDGE_TYPE_RIGHT   0x02u
#define UART_BRIDGE_PERIOD_MS    20u

/* CRC-8 over the 29-byte body (offsets 1..29) — poly 0x07, init 0xFF. */
uint8_t dash_crc8(const uint8_t *data, size_t n);

/* Encode dash → wire. Each writes exactly UART_BRIDGE_FRAME_LEN bytes. */
void dash_encode_left (uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq);
void dash_encode_right(uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq);

/* Decode wire → dash. Returns true on valid SOF/EOF/CRC and frame_type match. */
bool dash_decode_left (const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out);
bool dash_decode_right(const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out);

/* ── Menu identifiers ─────────────────────────────────────────────────────── */
#define DASH_MENU_NONE    0u
#define DASH_MENU_BOOST   1u   /* boost map selector */
#define DASH_MENU_TC      2u   /* traction-control slip-angle selector */

/* ── Threshold constants ───────────────────────────────────────────────────── */

/* RPM */
#define DASH_RPM_SHIFT_SLOW          5500.0f
#define DASH_RPM_SHIFT_FAST          6500.0f
#define DASH_RPM_LEDS_ALL_RED_ON     7000.0f   /* hysteresis: latch all-red    */
#define DASH_RPM_LEDS_ALL_RED_OFF    6500.0f   /* hysteresis: release all-red  */
/* DASH_RPM_REV_LIMIT: top of shift-LED display range (all 10 LEDs lit).
 * Display only shows live RPM from ECU — no rev-limiter state. */
#define DASH_RPM_REV_LIMIT           7500.0f

/* Oil */
#define DASH_OIL_TEMP_REDLINE        215.0f
#define DASH_OIL_TEMP_MAX            250.0f
#define DASH_OIL_PRESS_MIN           25.0f
#define DASH_OIL_PRESS_MIN_RPM       2000.0f
#define DASH_OIL_PRESS_REDLINE       125.0f

/* Fuel pressure — display range 20..160 PSI */
#define DASH_FUEL_PRESS_DISPLAY_MIN  20.0f
#define DASH_FUEL_PRESS_DISPLAY_MAX  160.0f
#define DASH_FUEL_PRESS_LOW          30.0f
#define DASH_FUEL_PRESS_REDLINE      125.0f

/* Coolant */
#define DASH_ECT_GREEN_MAX           200.0f
#define DASH_ECT_YELLOW_MAX          215.0f
#define DASH_ECT_ORANGE_MAX          230.0f
#define DASH_COOLANT_MAX             230.0f

/* IAT — display range 30..200 °F */
#define DASH_IAT_DISPLAY_MIN         30.0f
#define DASH_IAT_DISPLAY_MAX         200.0f
#define DASH_IAT_GREEN_MAX           100.0f
#define DASH_IAT_YELLOW_MAX          130.0f
#define DASH_IAT_ORANGE_MAX          150.0f

/* Boost */
#define DASH_BOOST_CYAN_MAX          14.0f
#define DASH_BOOST_ORANGE_MAX        20.0f
#define DASH_BOOST_OVERBOOST         22.0f

/* Lambda — widened for 93 octane AND E85 */
#define DASH_LAMBDA_RICH_ALARM       0.70f
#define DASH_LAMBDA_RICH_WARN        0.75f
#define DASH_LAMBDA_LEAN_WARN        1.02f
#define DASH_LAMBDA_LEAN_ALARM       1.05f

/* Ignition */
#define DASH_IGN_GREEN_MIN           12.0f
#define DASH_IGN_GREEN_MAX           32.0f

/* Fuel level */
#define DASH_FUEL_LOW                15.0f
#define DASH_FUEL_CAUTION            30.0f

/* UART staleness (side displays) */
#define DASH_STALE_MS                500u
#define DASH_STALE_OFF_MS            2000u

#ifdef __cplusplus
}
#endif

#endif /* TRACKCLUSTER_DASH_DATA_H */
