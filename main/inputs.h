/**
 * inputs.h — ODO button + dual rotary encoder handler, center cluster.
 *
 * Wires three physical inputs to LVGL events + CAN TX:
 *
 *   ODO button      Short press → cycle dash_odo_mode (ODO → TRIP A → TRIP B)
 *                   Long press  → reset the currently-displayed trip
 *                   (The ODO itself is never reset; it only counts up.)
 *
 *   Encoder 1 (Boost)
 *       Rotate → scroll BOOST MAP popup on right display (menu_id = DASH_MENU_BOOST)
 *       Press  → confirm selection, TX CAN frame ID CONFIG_TC_CAN_TX_BOOST_ID
 *
 *   Encoder 2 (TC)
 *       Rotate → scroll TC SLIP ANGLE popup on right display (menu_id = DASH_MENU_TC)
 *       Press  → confirm selection, TX CAN frame ID CONFIG_TC_CAN_TX_TC_ID
 *
 * All GPIO interrupts are debounced via esp_timer one-shots.
 * The LVGL indev is registered as LV_INDEV_TYPE_ENCODER so standard
 * LVGL focus/scroll events drive the popup widget on the right display's
 * data via the UART bridge — no LVGL object on the center display is needed
 * for the popup itself.
 */
#ifndef INPUTS_H
#define INPUTS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Call once from app_main() after bsp_init() and ui_init(). */
esp_err_t inputs_init(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUTS_H */
