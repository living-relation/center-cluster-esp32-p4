#pragma once
#include <stdint.h>
#include <stdbool.h>

void lap_timer_update(double lat, double lon, float speed_mph, bool has_fix);

uint64_t lap_timer_get_current_us(void);
uint64_t lap_timer_get_last_lap_us(void);
uint64_t lap_timer_get_best_us(void);
int32_t  lap_timer_get_delta_us(void);

void lap_timer_reset_session(void);