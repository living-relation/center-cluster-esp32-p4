/**
 * canbus.h — TWAI CAN bus driver for center cluster.
 *
 * Runs TWAI_MODE_NORMAL (bidirectional). Center receives ECU broadcast
 * frames AND transmits encoder selections (boost map, TC slip angle)
 * to the ECU on explicit button press — never on a timer.
 */
#ifndef CANBUS_H
#define CANBUS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * FreeRTOS task entry point. Pin to core 0 (CAN ISR affinity).
 *
 *   xTaskCreatePinnedToCore(canbus_task, "can", 8192, NULL, 7, NULL, 0);
 */
void canbus_task(void *arg);

/**
 * Transmit a 1-byte selection frame. id = CAN frame ID (e.g. 0x3EC for
 * boost map, 0x3ED for TC slip angle). index = selected item (0-based).
 * Call only from inputs.c on confirmed button press — never from a timer.
 */
void canbus_tx_selection(uint32_t id, uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* CANBUS_H */
