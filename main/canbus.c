/**
 * canbus.c — TWAI CAN bus driver, center cluster.
 *
 * Pins are configured in Kconfig.projbuild (TC_CAN_RX_GPIO / TC_CAN_TX_GPIO).
 * Default: RX=GPIO13, TX=GPIO14 — connect to an isolated CAN transceiver
 * (SN65HVD230 / ISO1050) whose CANH/CANL wires join the ECU CAN bus.
 *
 * Center runs TWAI_MODE_NORMAL — it receives ECU broadcast frames AND
 * transmits encoder selection frames (boost map, TC slip angle) to the ECU
 * on explicit button press. TX is never triggered by a timer.
 *
 * Link G4X PCLink custom-stream configuration: see protocols/link_g4x.json
 * and center-CANBUS-LINK-G4X-CONFIG.md for the exact channel assignments the
 * car owner must apply in PCLink before this firmware can decode anything.
 */

#include "canbus.h"
#include "dash_data.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

static const char *TAG = "canbus";

/* Critical-section mux shared with uart_bridge.c */
portMUX_TYPE g_dash_mux = portMUX_INITIALIZER_UNLOCKED;

/* ── Helpers ─────────────────────────────────────────────────────────── */
static inline uint16_t be_u16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static inline float c_to_f(int raw_c)   /* raw_c already has the −50 offset removed */
{
    return raw_c * 9.0f / 5.0f + 32.0f;
}

/* ── Frame decoders (big-endian, per Link G4X PCLink convention) ─────── */

/**
 * 0x3E8 — engine fast (10 ms)
 *   [0–1] RPM           u16 × 1
 *   [2–3] MAP absolute  u16 × 1 kPa   → derive boost = (MAP − 100) × 0.145038
 *   [4]   ECT           u8, °C + 50 offset
 *   [5]   IAT           u8, °C + 50 offset
 *   [6]   Oil temp      u8, °C + 50 offset
 *   [7]   reserved
 */
static void decode_3e8(const uint8_t *d)
{
    float map_kpa  = (float)be_u16(d + 2);
    float map_gauge = map_kpa - 100.0f;

    portENTER_CRITICAL(&g_dash_mux);
    g_dash.rpm          = (float)be_u16(d + 0);
    g_dash.boost        = map_gauge * 0.145038f;
    g_dash.coolant_temp = c_to_f((int)d[4] - 50);
    g_dash.iat          = c_to_f((int)d[5] - 50);
    g_dash.oil_temp     = c_to_f((int)d[6] - 50);
    portEXIT_CRITICAL(&g_dash_mux);
}

/**
 * 0x3E9 — speed / pressure / ignition (10 ms)
 *   [0–1] Ignition angle u16 × 0.1°, offset −100°
 *   [2]   Speed          u8 km/h → mph
 *   [3]   Oil pressure   u8 kPa  → PSI
 *   [4]   Fuel pressure  u8 kPa  → PSI
 *   [5–7] reserved
 */
static void decode_3e9(const uint8_t *d)
{
    float ign = (float)be_u16(d + 0) * 0.1f - 100.0f;

    portENTER_CRITICAL(&g_dash_mux);
    g_dash.ign_adv    = ign;
    g_dash.mph        = d[2] * 0.621371f;
    g_dash.oil_press  = d[3] * 0.145038f;
    g_dash.fuel_press = d[4] * 0.145038f;
    portEXIT_CRITICAL(&g_dash_mux);
}

/**
 * 0x3EA — lambda (10 ms)
 *   [0–1] Lambda 1  u16 × 0.001
 *   [2–7] reserved
 */
static void decode_3ea(const uint8_t *d)
{
    portENTER_CRITICAL(&g_dash_mux);
    g_dash.lambda = (float)be_u16(d + 0) * 0.001f;
    portEXIT_CRITICAL(&g_dash_mux);
}

/**
 * 0x3EB — gear / fuel level (50 ms)
 *   [0] Gear       u8  0=N, 1–6=fwd, 7=R (remapped to −1)
 *   [1] Fuel level u8  %
 *   [2–7] reserved
 */
static void decode_3eb(const uint8_t *d)
{
    portENTER_CRITICAL(&g_dash_mux);
    g_dash.gear       = (d[0] == 7) ? -1 : (int8_t)d[0];
    g_dash.fuel_level = (float)d[1];
    portEXIT_CRITICAL(&g_dash_mux);
}

/**
 * 0x3EE — engine-protection status (50 ms)
 *   [0] knock  [1] ign-cut  [2] fuel-cut  [3] boost-cut  [4] sensor-err  [5] throttle-err
 *   Each byte: 0 = OK, nonzero = active. ORed into g_dash.warn (right-cluster overlay).
 *   Conditions already shown on gauges (oil press / fuel / temps) are NOT carried here.
 */
static void decode_3ee(const uint8_t *d)
{
    uint8_t w = 0;
    if (d[0]) w |= DASH_WARN_KNOCK;
    if (d[1]) w |= DASH_WARN_IGN_CUT;
    if (d[2]) w |= DASH_WARN_FUEL_CUT;
    if (d[3]) w |= DASH_WARN_BOOST_CUT;
    if (d[4]) w |= DASH_WARN_SENSOR;
    if (d[5]) w |= DASH_WARN_THROTTLE;
    portENTER_CRITICAL(&g_dash_mux);
    g_dash.warn = w;
    portEXIT_CRITICAL(&g_dash_mux);
}

/* ── TWAI installation ───────────────────────────────────────────────── */
static esp_err_t twai_install(void)
{
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
        CONFIG_TC_CAN_TX_GPIO, CONFIG_TC_CAN_RX_GPIO, TWAI_MODE_NORMAL);
    g.rx_queue_len = 32;
    g.tx_queue_len = 4;   /* small TX queue — only encoder selection frames */

    twai_timing_config_t t = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_RETURN_ON_ERROR(twai_driver_install(&g, &t, &f), TAG, "install");
    ESP_RETURN_ON_ERROR(twai_start(), TAG, "start");
    ESP_LOGI(TAG, "TWAI started — NORMAL mode, 1 Mbit/s, TX=%d RX=%d",
             CONFIG_TC_CAN_TX_GPIO, CONFIG_TC_CAN_RX_GPIO);
    return ESP_OK;
}

/* ── Task ────────────────────────────────────────────────────────────── */
void canbus_task(void *arg)
{
    ESP_ERROR_CHECK(twai_install());

    twai_message_t msg;
    for (;;) {
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) != ESP_OK) {
            /* Timeout — no frame in 100 ms. Bus may be quiet at idle; not an error. */
            continue;
        }
        if (msg.rtr) continue;  /* ignore remote-transmission requests */

        /* Stamp freshness before decode so side clusters see it immediately */
        portENTER_CRITICAL(&g_dash_mux);
        g_dash.last_update_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        portEXIT_CRITICAL(&g_dash_mux);

        switch (msg.identifier) {
            case 0x3E8: decode_3e8(msg.data); break;
            case 0x3E9: decode_3e9(msg.data); break;
            case 0x3EA: decode_3ea(msg.data); break;
            case 0x3EB: decode_3eb(msg.data); break;
            case 0x3EE: decode_3ee(msg.data); break;
            default:    /* defensive: no other IDs expected from Link G4X stream */  break;
        }
    }
}

/* ── TX: encoder selection ───────────────────────────────────────────── */
void canbus_tx_selection(uint32_t id, uint8_t index)
{
    twai_message_t msg = {
        .identifier       = id,
        .data_length_code = 1,
        .data             = { index, 0, 0, 0, 0, 0, 0, 0 },
        .flags            = 0,
    };
    esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(10));
    if (err != ESP_OK)
        ESP_LOGW(TAG, "TX failed id=0x%03" PRIx32 " err=%d", id, err);
    else
        ESP_LOGI(TAG, "TX id=0x%03" PRIx32 " data=%u", id, index);
}
