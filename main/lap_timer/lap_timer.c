#include "lap_timer.h"
#include "esp_timer.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/*
    Ridge Motorsports Park Start/Finish Line
    47.254635502496825, -123.19114911865732
*/


#define FINISH_LAT 47.254635
#define FINISH_LON -123.191149

#define FINISH_RADIUS_M     10.0        // Trigger radius
#define MIN_LAP_TIME_US     (30ULL * 1000000ULL)
#define MIN_SPEED_MPH       10.0f
#define CROSS_DEBOUNCE_US   (10ULL * 1000000ULL)

#define EARTH_RADIUS_M      6371000.0

// ---------------------------
// Internal State
// ---------------------------

static bool     lap_running      = false;
static uint64_t lap_start_us     = 0;
static uint64_t last_cross_us    = 0;

static uint64_t current_lap_us   = 0;
static uint64_t best_lap_us      = 0;

// Predictive timing
static int32_t  predictive_delta_us = 0;

// Reference ghost lap
static uint64_t reference_lap_us = 0;


static double deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

static double distance_m(double lat1, double lon1, double lat2, double lon2) {
    double dLat = deg_to_rad(lat2 - lat1);
    double dLon = deg_to_rad(lon2 - lon1);

    double a = sin(dLat/2)*sin(dLat/2) +
               cos(deg_to_rad(lat1)) *
               cos(deg_to_rad(lat2)) *
               sin(dLon/2)*sin(dLon/2);

    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return EARTH_RADIUS_M * c;
}

void lap_timer_update(double lat, double lon, float speed_mph, bool has_fix) {
    if (!has_fix)
        return;

    if (speed_mph < MIN_SPEED_MPH)
        return;

    uint64_t now = esp_timer_get_time();

    // Distance from finish
    double dist = distance_m(lat, lon, FINISH_LAT, FINISH_LON);

    if (dist > FINISH_RADIUS_M)
        return;

    // Debounce to prevent multi-trigger
    if (now - last_cross_us < CROSS_DEBOUNCE_US)
        return;

    last_cross_us = now;

    // First ever crossing = start timer
    if (!lap_running) {
        lap_running = true;
        lap_start_us = now;
        return;
    }

    uint64_t lap_time = now - lap_start_us;

    // Reject unrealistic lap
    if (lap_time < MIN_LAP_TIME_US)
        return;

    current_lap_us = lap_time;

    // Update best lap
    if (best_lap_us == 0 || lap_time < best_lap_us) {
        best_lap_us = lap_time;
        reference_lap_us = lap_time;
    }

    // Reset for next lap
    lap_start_us = now;
}

uint64_t lap_timer_get_current_us(void) {
    if (!lap_running)
        return 0;

    return esp_timer_get_time() - lap_start_us;
}

uint64_t lap_timer_get_last_lap_us(void) {
    return current_lap_us;
}

uint64_t lap_timer_get_best_us(void) {
    return best_lap_us;
}

int32_t lap_timer_get_delta_us(void) {
    if (!lap_running || reference_lap_us == 0)
        return 0;

    uint64_t current_elapsed = lap_timer_get_current_us();

    if (current_elapsed > reference_lap_us)
        predictive_delta_us = current_elapsed - reference_lap_us;
    else
        predictive_delta_us = -(int32_t)(reference_lap_us - current_elapsed);

    return predictive_delta_us;
}

void lap_timer_reset_session(void) {
    lap_running = false;
    lap_start_us = 0;
    last_cross_us = 0;
    current_lap_us = 0;
    best_lap_us = 0;
    reference_lap_us = 0;
    predictive_delta_us = 0;
}