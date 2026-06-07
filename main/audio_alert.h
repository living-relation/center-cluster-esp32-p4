/**
 * audio_alert.h — ES8311 speaker tones on the center P4 board.
 */
#ifndef AUDIO_ALERT_H
#define AUDIO_ALERT_H

#include "esp_err.h"

/** Init I2C + I2S + ES8311 once (no-op if already ready). */
esp_err_t audio_alert_init(void);

/** Non-blocking: boot chime mid-splash (5 kHz, 4 equal sample-timed beeps). */
void audio_alert_play_boot_async(void);

/** Background monitor: warning tone on rising edge of g_dash.warn (CAN 0x3EE). */
void audio_alert_warn_monitor_start(void);

#endif /* AUDIO_ALERT_H */
