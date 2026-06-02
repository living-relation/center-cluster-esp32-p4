/**
 * uart_bridge.h — dual UART TX bridge (center → left/right).
 */
#ifndef UART_BRIDGE_H
#define UART_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * FreeRTOS task: encodes and transmits LEFT + RIGHT UART frames every 20 ms.
 * Pin-to-core 0 recommended (matches CAN task core, avoids cross-core mux).
 *
 *   xTaskCreatePinnedToCore(uart_tx_task, "uart", 4096, NULL, 6, NULL, 0);
 */
void uart_tx_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* UART_BRIDGE_H */
