/**
 * dash_data.c — CRC-8 + LEFT/RIGHT encoder + decoder.
 *
 * Shared, byte-for-byte identical across all three firmware projects.
 */

#include "dash_data.h"
#include <string.h>

/* Globally available data snapshot. */
volatile dash_data_t g_dash;

/* ── CRC-8 (poly 0x07, init 0xFF) ──────────────────────────────────────── */
uint8_t dash_crc8(const uint8_t *data, size_t n)
{
    uint8_t c = 0xFF;
    for (size_t i = 0; i < n; i++) {
        c ^= data[i];
        for (int b = 0; b < 8; b++) {
            c = (c & 0x80u) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
        }
    }
    return c;
}

/* ── Little-endian writers ─────────────────────────────────────────────── */
static inline void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v;        p[1] = (uint8_t)(v >> 8); }
static inline void put_i16(uint8_t *p, int16_t  v) { put_u16(p, (uint16_t)v); }
static inline void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline uint16_t get_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static inline int16_t  get_i16(const uint8_t *p) { return (int16_t)get_u16(p); }
static inline uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* Clamp helpers used to keep float→fixed conversions safely in range. */
static inline uint16_t clamp_u16(float v) { return (v < 0.0f) ? 0u : (v > 65535.0f) ? 65535u : (uint16_t)v; }
static inline int16_t  clamp_i16(float v) { return (v < -32768.0f) ? (int16_t)-32768 : (v > 32767.0f) ? (int16_t)32767 : (int16_t)v; }
static inline uint32_t clamp_u32(float v) { return (v < 0.0f) ? 0u : (v > 4294967295.0f) ? 0xFFFFFFFFu : (uint32_t)v; }

/* ── LEFT encoder ──────────────────────────────────────────────────────── */
void dash_encode_left(uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq)
{
    memset(out, 0, UART_BRIDGE_FRAME_LEN);
    out[0] = UART_BRIDGE_SOF;
    out[1] = UART_BRIDGE_TYPE_LEFT;
    put_u16(out + 2, seq);

    uint8_t *p = out + 4;
    put_u16(p + 0,  clamp_u16(d->mph        * 10.0f));
    put_i16(p + 2,  clamp_i16(d->oil_temp   * 10.0f));
    put_u16(p + 4,  clamp_u16(d->oil_press  * 10.0f));
    put_u16(p + 6,  clamp_u16(d->fuel_press * 10.0f));
    put_u16(p + 8,  clamp_u16(d->fuel_level * 10.0f));
    put_u16(p + 10, clamp_u16(d->rpm));
    p[12] = (uint8_t)d->gear;
    p[13] = d->odo_mode;
    put_u32(p + 14, clamp_u32(d->odo * 10.0f));

    float trip = (d->odo_mode == DASH_TRIP_A) ? d->trip_a
              : (d->odo_mode == DASH_TRIP_B) ? d->trip_b
              :                                d->odo;
    put_u32(p + 18, clamp_u32(trip * 10.0f));
    put_u16(p + 22, d->flags);
    p[24] = d->brightness;   /* frame offset 28 — shared backlight level */
    /* p+25 reserved 0 (memset already cleared) */

    out[30] = dash_crc8(out + 1, 29);
    out[31] = UART_BRIDGE_EOF;
}

/* ── RIGHT encoder ─────────────────────────────────────────────────────── */
void dash_encode_right(uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq)
{
    memset(out, 0, UART_BRIDGE_FRAME_LEN);
    out[0] = UART_BRIDGE_SOF;
    out[1] = UART_BRIDGE_TYPE_RIGHT;
    put_u16(out + 2, seq);

    uint8_t *p = out + 4;
    put_u16(p + 0, clamp_u16(d->lambda * 1000.0f));
    /* Boost is biased by +30 so vacuum encodes as a positive value */
    put_i16(p + 2, clamp_i16((d->boost + 30.0f) * 100.0f));
    put_i16(p + 4, clamp_i16(d->coolant_temp * 10.0f));
    put_i16(p + 6, clamp_i16(d->ign_adv      * 10.0f));
    put_i16(p + 8, clamp_i16(d->iat          * 10.0f));
    put_u16(p + 10, clamp_u16(d->rpm));
    put_u16(p + 12, d->flags);
    /* bytes 14-15: menu state (encoder popup forwarded to right display) */
    p[14] = d->menu_id;
    p[15] = d->menu_cursor;
    /* byte 16: engine-protection warnings (drives right warning overlay) */
    p[16] = d->warn;
    p[24] = d->brightness;   /* frame offset 28 — shared backlight level */
    /* p+17..23, p+25 reserved 0 */

    out[30] = dash_crc8(out + 1, 29);
    out[31] = UART_BRIDGE_EOF;
}

/* ── Decoders ──────────────────────────────────────────────────────────── */
static bool decode_common(const uint8_t in[UART_BRIDGE_FRAME_LEN], uint8_t expect_type, uint16_t *seq_out)
{
    if (in[0]  != UART_BRIDGE_SOF)       return false;
    if (in[1]  != expect_type)           return false;
    if (in[31] != UART_BRIDGE_EOF)       return false;
    if (dash_crc8(in + 1, 29) != in[30]) return false;
    if (seq_out) *seq_out = get_u16(in + 2);
    return true;
}

bool dash_decode_left(const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out)
{
    if (!decode_common(in, UART_BRIDGE_TYPE_LEFT, seq_out)) return false;
    const uint8_t *p = in + 4;

    d->mph         = (float)get_u16(p + 0)  / 10.0f;
    d->oil_temp    = (float)get_i16(p + 2)  / 10.0f;
    d->oil_press   = (float)get_u16(p + 4)  / 10.0f;
    d->fuel_press  = (float)get_u16(p + 6)  / 10.0f;
    d->fuel_level  = (float)get_u16(p + 8)  / 10.0f;
    d->rpm         = (float)get_u16(p + 10);
    d->gear        = (int8_t)p[12];
    d->odo_mode    = p[13];
    d->odo         = (float)get_u32(p + 14) / 10.0f;

    float trip = (float)get_u32(p + 18) / 10.0f;
    switch (d->odo_mode) {
        case DASH_TRIP_A: d->trip_a = trip; break;
        case DASH_TRIP_B: d->trip_b = trip; break;
        default:          d->odo    = trip; break;
    }
    d->flags = get_u16(p + 22);
    d->brightness = p[24];   /* frame offset 28 — shared backlight level */
    return true;
}

bool dash_decode_right(const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out)
{
    if (!decode_common(in, UART_BRIDGE_TYPE_RIGHT, seq_out)) return false;
    const uint8_t *p = in + 4;

    d->lambda       = (float)get_u16(p + 0) / 1000.0f;
    d->boost        = (float)get_i16(p + 2) / 100.0f - 30.0f;   /* un-bias */
    d->coolant_temp = (float)get_i16(p + 4) / 10.0f;
    d->ign_adv      = (float)get_i16(p + 6) / 10.0f;
    d->iat          = (float)get_i16(p + 8) / 10.0f;
    d->rpm          = (float)get_u16(p + 10);
    d->flags        = get_u16(p + 12);
    d->menu_id      = p[14];
    d->menu_cursor  = p[15];
    d->warn         = p[16];
    d->brightness   = p[24];   /* frame offset 28 — shared backlight level */
    return true;
}
