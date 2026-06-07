#pragma once
#include <stdint.h>
#include "esp_err.h"

esp_err_t can_sim_console_register(void);

/** Inject knock warning once (clear then 0x3EE byte0=1) for bench testing. */
void can_sim_trigger_warning_once(void);
