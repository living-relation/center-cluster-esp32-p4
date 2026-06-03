/**
 * main.c — center cluster application entry point.
 *
 * Boot sequence:
 *   1. nvs_flash_init()    — odometer persistence
 *   2. bsp_init()          — MIPI-DSI panel bring-up
 *   3. bsp_display_start() — LVGL port + display registration
 *   4. ui_init()           — LVGL widget tree (boot splash → live screen)
 *   5. NORMAL: inputs_init() + canbus_task + uart_tx_task   (real hardware)
 *      BENCH:  bench_task (CONFIG_TC_BENCH_MODE — standalone demo sweep)
 *   6. alarm_task          — 20 ms threshold scanner → g_dash.flags (both modes)
 *
 * The center has no "no-data" overlay: with nothing connected the gauges simply
 * render g_dash at 0. Bench mode sweeps them so every widget can be verified.
 */

#include "bsp.h"
#include "canbus.h"
#include "uart_bridge.h"
#include "inputs.h"
#include "dash_data.h"
#include "ui/ui.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include <math.h>

static const char *TAG = "main";

extern void alarm_task(void *arg);

#if CONFIG_TC_BENCH_MODE
extern portMUX_TYPE g_dash_mux;
/* Bench mode: no ECU connected. Slowly sweep the center's channels so the tach
 * arc, shift LEDs, gear box and odometer can be verified standalone. */
static void bench_task(void *arg)
{
    const TickType_t bench_period = pdMS_TO_TICKS(4);  /* 250 Hz producer cadence */
    const float half_cycle_s = 2.0f;                   /* faster 0->max / max->0 */
    const float p_step = 0.004f / half_cycle_s;        /* progress step per tick */
    const uint32_t hold_max_ms = 1000;                 /* hold 1s at peak RPM */
    const uint32_t hold_max_steps = hold_max_ms / 4;
    const uint32_t min_shift_interval_ms = 280;      /* avoid rapid gear hopping */

    TickType_t wake = xTaskGetTickCount();
    float p = 0.0f;
    int dir = 1;
    uint32_t hold_steps = 0;
    int8_t gear = 1;
    uint32_t last_shift_ms = 0;

    for (;;) {
        if (hold_steps > 0) {
            hold_steps--;
            p = 1.0f;
        } else {
            p += (dir > 0) ? p_step : -p_step;
            if (p >= 1.0f) {
                p = 1.0f;    /* exact top sample */
                hold_steps = hold_max_steps;
                dir = -1;
            } else if (p <= 0.0f) {
                p = 0.0f;    /* exact bottom sample */
                dir = 1;
            }
        }

        /* Linear reflected ramp for constant dRPM/dt across the sweep. */
        const float rpm = p * 8000.0f;                       /* full 0..8000 range */
        const float mph = p * 120.0f;                        /* 0..120 */

        const uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if ((now_ms - last_shift_ms) >= min_shift_interval_ms) {
            /* Simple hysteretic shift model tied to RPM to keep gear changes
             * progressive and visually realistic. */
            if (gear < 6 && rpm > 6400.0f) {
                gear++;
                last_shift_ms = now_ms;
            } else if (gear > 1 && rpm < 2200.0f) {
                gear--;
                last_shift_ms = now_ms;
            }
        }

        portENTER_CRITICAL(&g_dash_mux);
        g_dash.rpm            = rpm;
        g_dash.mph            = mph;
        g_dash.gear           = gear;
        g_dash.last_update_ms = now_ms;
        portEXIT_CRITICAL(&g_dash_mux);

        vTaskDelayUntil(&wake, bench_period);
    }
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "TrackCluster Center — booting");

    /* NVS (odometer persistence) */
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition erased — odometer reset");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    /* Board bring-up */
    ESP_ERROR_CHECK(bsp_init());
    lv_disp_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "LVGL display init failed — halting");
        for (;;) vTaskDelay(portMAX_DELAY);
    }

    /* UI */
    bsp_lvgl_lock(portMAX_DELAY);
    ui_init(disp);
    bsp_lvgl_unlock();

#if CONFIG_TC_BENCH_MODE
    ESP_LOGW(TAG, "BENCH MODE — demo sweep; CAN/UART/inputs disabled");
    xTaskCreatePinnedToCore(bench_task, "bench", 4096, NULL, 4, NULL, 0);
#else
    /* Inputs: ODO button + dual rotary encoders (GPIO ISRs + esp_timer debounce) */
    ESP_ERROR_CHECK(inputs_init());
    xTaskCreatePinnedToCore(canbus_task,  "can",   8192, NULL, 7, NULL, 0);
    xTaskCreatePinnedToCore(uart_tx_task, "uart",  4096, NULL, 6, NULL, 0);
#endif
    xTaskCreatePinnedToCore(alarm_task,   "alarm", 4096, NULL, 6, NULL, 0);

    ESP_LOGI(TAG, "All tasks started");
}
