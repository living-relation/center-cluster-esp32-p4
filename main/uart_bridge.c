/**
 * uart_bridge.c — dual UART TX bridge, center cluster.
 *
 * Encodes dash_data_t subsets into 32-byte LEFT / RIGHT frames and ships
 * them over two UARTs at 921 600 8N1, 50 Hz cadence.
 *
 * Pin defaults (Kconfig.projbuild):
 *   UART1 TX = GPIO20 → Left S3  GPIO44 RX
 *   UART2 TX = GPIO21 → Right S3 GPIO44 RX
 *
 * Frame format and encoder/decoder implementations live in shared/dash_data.c.
 * This file owns only the UART driver bring-up and the TX task loop.
 */

#include "uart_bridge.h"
#include "dash_data.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "uart_bridge";

/* Declared in canbus.c; shared for the atomic snapshot. */
extern portMUX_TYPE g_dash_mux;

/* ── UART driver bring-up ────────────────────────────────────────────── */
static void uart_init(uart_port_t port, int tx_gpio, int rx_gpio)
{
    uart_config_t cfg = {
        .baud_rate  = UART_BRIDGE_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_gpio, rx_gpio,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    /* 256-byte RX buf is minimal — we only TX here; RX is reserved future. */
    ESP_ERROR_CHECK(uart_driver_install(port, 256, 256, 0, NULL, 0));
    ESP_LOGI(TAG, "UART%d ready — baud=%u, TX=GPIO%d RX=GPIO%d",
             (int)port, UART_BRIDGE_BAUD, tx_gpio, rx_gpio);
}

/* ── TX task ─────────────────────────────────────────────────────────── */
void uart_tx_task(void *arg)
{
    uart_init(UART_NUM_1,
              CONFIG_TC_UART1_TX_GPIO, CONFIG_TC_UART1_RX_GPIO);   /* → LEFT  */
    uart_init(UART_NUM_2,
              CONFIG_TC_UART2_TX_GPIO, CONFIG_TC_UART2_RX_GPIO);   /* → RIGHT */

    uint8_t  buf_l[UART_BRIDGE_FRAME_LEN];
    uint8_t  buf_r[UART_BRIDGE_FRAME_LEN];
    uint16_t seq_l = 0, seq_r = 0;

    TickType_t    wake   = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(UART_BRIDGE_PERIOD_MS);

    for (;;) {
        /* Atomic snapshot of the global dash struct */
        dash_data_t snap;
        portENTER_CRITICAL(&g_dash_mux);
        snap = *(const dash_data_t *)&g_dash;
        portEXIT_CRITICAL(&g_dash_mux);

        dash_encode_left (buf_l, &snap, seq_l++);
        dash_encode_right(buf_r, &snap, seq_r++);

        uart_write_bytes(UART_NUM_1, (const char *)buf_l, UART_BRIDGE_FRAME_LEN);
        uart_write_bytes(UART_NUM_2, (const char *)buf_r, UART_BRIDGE_FRAME_LEN);

        /* Strict 20 ms cadence — compensates for any encode overhead */
        vTaskDelayUntil(&wake, period);
    }
}
