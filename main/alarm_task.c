/**
 * alarm_task.c — 20 ms threshold scanner, center cluster.
 *
 * Runs independently of the 33 ms LVGL paint tick. Evaluates displayed-channel
 * thresholds and writes DASH_FLAG_* bits into g_dash.flags so both the local
 * UI and the UART bridge pick them up atomically on the next tick.
 *
 * End-to-end latency budget (ECU frame → pixel alarm):
 *   CAN cadence (10–20 ms) + ISR stamp (<200 µs) + THIS task (0–20 ms)
 *   + UART hop (32 B @ 921 600 = ~350 µs) + LVGL flush (≤16 ms)
 *   ≈ 30–56 ms worst case — within the <50 ms target for most transitions.
 */

#include "dash_data.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "esp_log.h"

extern portMUX_TYPE g_dash_mux;

void alarm_task(void *arg)
{
    TickType_t    wake   = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);

    for (;;) {
        /* Snapshot */
        dash_data_t s;
        portENTER_CRITICAL(&g_dash_mux);
        s = *(const dash_data_t *)&g_dash;
        portEXIT_CRITICAL(&g_dash_mux);

        uint16_t flags = 0;

        /* Oil pressure low (only meaningful above idle) */
        if (s.rpm > DASH_OIL_PRESS_MIN_RPM && s.oil_press < DASH_OIL_PRESS_MIN)
            flags |= DASH_FLAG_OIL_PRESS_LOW;

        /* Fuel level low */
        if (s.fuel_level < DASH_FUEL_LOW)
            flags |= DASH_FLAG_FUEL_LOW;

        /* Shift light (drives flash gate) */
        if (s.rpm >= DASH_RPM_SHIFT_SLOW)
            flags |= DASH_FLAG_SHIFT;

        /* Coolant hot */
        if (s.coolant_temp >= DASH_COOLANT_MAX)
            flags |= DASH_FLAG_COOLANT_HOT;

        /* Oil temp hot */
        if (s.oil_temp >= DASH_OIL_TEMP_REDLINE)
            flags |= DASH_FLAG_OIL_TEMP_HOT;

        /* Lambda alarm (no throttle gating — throttle not in data model) */
        if (s.lambda < DASH_LAMBDA_RICH_ALARM || s.lambda >= DASH_LAMBDA_LEAN_ALARM)
            flags |= DASH_FLAG_LAMBDA_BAD;

        /* Overboost */
        if (s.boost >= DASH_BOOST_OVERBOOST)
            flags |= DASH_FLAG_OVERBOOST;

        /* Write back atomically */
        portENTER_CRITICAL(&g_dash_mux);
        g_dash.flags = flags;
        portEXIT_CRITICAL(&g_dash_mux);

        vTaskDelayUntil(&wake, period);
    }
}
