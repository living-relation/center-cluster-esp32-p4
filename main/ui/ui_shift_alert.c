/**
 * ui_shift_alert.c — shift strobe timing (gear box + shift LEDs).
 *
 * Strobe begins at 6000 RPM; toggle rate increases every 200 RPM to ~20 Hz at 8000.
 */
#include "ui_shift_alert.h"
#include "center-colors.h"

#define STROBE_RPM_ON    6000.0f
#define STROBE_RPM_TOP   8000.0f
#define STROBE_RPM_STEP  200.0f
#define STROBE_HZ_AT_ON  4.0f
#define STROBE_HZ_AT_TOP 20.0f

static bool     s_all_red = false;
static bool     s_flash_on = true;
static bool     s_strobe = false;
static uint32_t s_last_toggle_ms = 0;

static uint32_t strobe_period_ms(float rpm)
{
    if (rpm < STROBE_RPM_ON) {
        return 0;
    }
    float steps = (rpm - STROBE_RPM_ON) / STROBE_RPM_STEP;
    if (steps < 0.0f) {
        steps = 0.0f;
    }
    if (steps > 10.0f) {
        steps = 10.0f;
    }
    float hz = STROBE_HZ_AT_ON + steps * ((STROBE_HZ_AT_TOP - STROBE_HZ_AT_ON) / 10.0f);
    if (hz < 1.0f) {
        hz = 1.0f;
    }
    return (uint32_t)(1000.0f / hz);
}

void ui_shift_alert_update(const dash_data_t *d)
{
    if (!s_all_red && d->rpm >= DASH_RPM_LEDS_ALL_RED_ON) {
        s_all_red = true;
    }
    if (s_all_red && d->rpm < DASH_RPM_LEDS_ALL_RED_OFF) {
        s_all_red = false;
    }

    uint32_t now = lv_tick_get();
    s_strobe = (d->rpm >= STROBE_RPM_ON);

    if (s_strobe) {
        uint32_t period = strobe_period_ms(d->rpm);
        if (period > 0 && (now - s_last_toggle_ms) >= period) {
            s_flash_on = !s_flash_on;
            s_last_toggle_ms = now;
        }
    } else {
        s_flash_on = true;
    }
}

bool ui_shift_alert_strobe_active(void)
{
    return s_strobe;
}

bool ui_shift_alert_flash_on(void)
{
    return s_flash_on;
}

bool ui_shift_alert_all_red(void)
{
    return s_all_red;
}
