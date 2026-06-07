/**
 * audio_alert.c — sine-wave beeps via onboard ES8311 + I2S speaker (Waveshare P4 XC).
 *
 * Boot:  5000 Hz, 4 equal beeps starting mid-splash (sample-accurate timing).
 * Warn:  3900 Hz when ECU frame 0x3EE sets g_dash.warn (same byte UART-fwd to right).
 */
#include "audio_alert.h"
#include "dash_data.h"
#include "ui/ui_boot.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "es8311.h"
#include "esp_check.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG = "audio";

extern portMUX_TYPE g_dash_mux;

/* Waveshare ESP32-P4-WIFI6-Touch-LCD-XC: ES8311 on I2C0 SDA=7 SCL=8 (Waveshare + IDF P4 example). */
#define AUDIO_I2C_PORT      I2C_NUM_0
#define AUDIO_I2C_SDA       GPIO_NUM_7
#define AUDIO_I2C_SCL       GPIO_NUM_8
#define AUDIO_I2S_MCK       GPIO_NUM_13
#define AUDIO_I2S_BCK       GPIO_NUM_12
#define AUDIO_I2S_WS        GPIO_NUM_10
#define AUDIO_I2S_DOUT      GPIO_NUM_9
#define AUDIO_I2S_DIN       GPIO_NUM_11
#define AUDIO_PA_GPIO       GPIO_NUM_53

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_MCLK_MULT     384
#define AUDIO_VOICE_VOL     70
#define AUDIO_CHUNK_SAMPLES 256

#define BOOT_FREQ_HZ        5000.0f
#define BOOT_BEEP_COUNT     4
#define BOOT_TONE_MS        120
#define BOOT_GAP_MS         120

#define WARN_FREQ_HZ        3900.0f
#define WARN_BEEPS_PER_S    5
#define WARN_MAX_MS         5000

static i2s_chan_handle_t s_i2s_tx = NULL;
static es8311_handle_t   s_codec   = NULL;
static bool              s_ready   = false;

static size_t ms_to_frames(unsigned ms)
{
    return ((size_t)ms * (size_t)AUDIO_SAMPLE_RATE) / 1000u;
}

static esp_err_t pa_enable(bool on)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << AUDIO_PA_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    static bool gpio_done = false;
    if (!gpio_done) {
        ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "pa_gpio");
        gpio_done = true;
    }
    gpio_set_level(AUDIO_PA_GPIO, on ? 1 : 0);
    return ESP_OK;
}

static esp_err_t codec_hw_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    const i2c_config_t i2c_cfg = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = AUDIO_I2C_SDA,
        .scl_io_num       = AUDIO_I2C_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(AUDIO_I2C_PORT, &i2c_cfg), TAG, "i2c_cfg");
    esp_err_t err = i2c_driver_install(AUDIO_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    s_codec = es8311_create(AUDIO_I2C_PORT, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(s_codec, ESP_FAIL, TAG, "es8311_create");

    const es8311_clock_config_t clk = {
        .mclk_inverted      = false,
        .sclk_inverted      = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency     = AUDIO_SAMPLE_RATE * AUDIO_MCLK_MULT,
        .sample_frequency   = AUDIO_SAMPLE_RATE,
    };
    ESP_RETURN_ON_ERROR(es8311_init(s_codec, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16), TAG, "es8311_init");
    ESP_RETURN_ON_ERROR(
        es8311_sample_frequency_config(s_codec, AUDIO_SAMPLE_RATE * AUDIO_MCLK_MULT, AUDIO_SAMPLE_RATE),
        TAG, "es8311_fs");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(s_codec, AUDIO_VOICE_VOL, NULL), TAG, "es8311_vol");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(s_codec, false), TAG, "es8311_mic");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_i2s_tx, NULL), TAG, "i2s_chan");

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = AUDIO_I2S_MCK,
            .bclk = AUDIO_I2S_BCK,
            .ws   = AUDIO_I2S_WS,
            .dout = AUDIO_I2S_DOUT,
            .din  = AUDIO_I2S_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = AUDIO_MCLK_MULT;
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_i2s_tx, &std_cfg), TAG, "i2s_std");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_i2s_tx), TAG, "i2s_en");

    ESP_RETURN_ON_ERROR(pa_enable(true), TAG, "pa_on");
    s_ready = true;
    ESP_LOGI(TAG, "ES8311 speaker ready");
    return ESP_OK;
}

static void fill_sine_stereo(int16_t *stereo, size_t frames, float freq_hz, float amp, float *phase)
{
    const float two_pi = 6.28318530718f;
    for (size_t i = 0; i < frames; i++) {
        float s = sinf(two_pi * freq_hz * (*phase) / (float)AUDIO_SAMPLE_RATE);
        int32_t v = (int32_t)(amp * 32767.0f * s);
        if (v > 32767) {
            v = 32767;
        } else if (v < -32768) {
            v = -32768;
        }
        stereo[i * 2]     = (int16_t)v;
        stereo[i * 2 + 1] = (int16_t)v;
        *phase += 1.0f;
    }
}

static esp_err_t write_pcm(const int16_t *stereo, size_t frames)
{
    size_t bytes = frames * 2 * sizeof(int16_t);
    size_t written = 0;
    return i2s_channel_write(s_i2s_tx, stereo, bytes, &written, portMAX_DELAY);
}

static void play_tone_frames(float freq_hz, size_t frames, bool (*should_stop)(void))
{
    if (!s_ready || frames == 0) {
        return;
    }

    int16_t buf[AUDIO_CHUNK_SAMPLES * 2];
    float phase = 0.0f;
    size_t done = 0;

    while (done < frames) {
        if (should_stop && should_stop()) {
            return;
        }
        size_t chunk = frames - done;
        if (chunk > AUDIO_CHUNK_SAMPLES) {
            chunk = AUDIO_CHUNK_SAMPLES;
        }
        fill_sine_stereo(buf, chunk, freq_hz, 0.35f, &phase);
        if (write_pcm(buf, chunk) != ESP_OK) {
            return;
        }
        done += chunk;
    }
}

static void play_silence_frames(size_t frames)
{
    if (!s_ready || frames == 0) {
        return;
    }

    int16_t buf[AUDIO_CHUNK_SAMPLES * 2];
    memset(buf, 0, sizeof(buf));
    size_t done = 0;

    while (done < frames) {
        size_t chunk = frames - done;
        if (chunk > AUDIO_CHUNK_SAMPLES) {
            chunk = AUDIO_CHUNK_SAMPLES;
        }
        if (write_pcm(buf, chunk) != ESP_OK) {
            return;
        }
        done += chunk;
    }
}

static void play_beep_count(float freq_hz, unsigned count, unsigned tone_ms, unsigned gap_ms,
                            bool (*should_stop)(void))
{
    if (!s_ready || count == 0 || tone_ms == 0) {
        return;
    }

    const size_t tone_frames = ms_to_frames(tone_ms);
    const size_t gap_frames  = ms_to_frames(gap_ms);

    for (unsigned i = 0; i < count; i++) {
        if (should_stop && should_stop()) {
            return;
        }
        play_tone_frames(freq_hz, tone_frames, should_stop);
        if (i + 1 < count && gap_frames > 0) {
            play_silence_frames(gap_frames);
        }
    }
}

static void play_beep_train(float freq_hz, unsigned beeps_per_sec, unsigned total_ms,
                            bool (*should_stop)(void))
{
    if (!s_ready || beeps_per_sec == 0 || total_ms == 0) {
        return;
    }

    const unsigned period_ms = 1000u / beeps_per_sec;
    const unsigned tone_ms   = period_ms / 2;
    const unsigned gap_ms    = period_ms - tone_ms;
    if (tone_ms == 0) {
        return;
    }

    const uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    while (((uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - t0) < total_ms) {
        if (should_stop && should_stop()) {
            break;
        }
        play_beep_count(freq_hz, 1, tone_ms, 0, should_stop);
        if (should_stop && should_stop()) {
            return;
        }
        play_silence_frames(ms_to_frames(gap_ms));
    }
}

static bool warn_cleared_cb(void)
{
    uint8_t w;
    portENTER_CRITICAL(&g_dash_mux);
    w = g_dash.warn;
    portEXIT_CRITICAL(&g_dash_mux);
    return w == 0;
}

static void boot_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(UI_BOOT_CHIME_START_MS));

    if (audio_alert_init() != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }
    play_beep_count(BOOT_FREQ_HZ, BOOT_BEEP_COUNT, BOOT_TONE_MS, BOOT_GAP_MS, NULL);
    vTaskDelete(NULL);
}

static void warn_monitor_task(void *arg)
{
    (void)arg;
    uint8_t prev = 0;

    for (;;) {
        uint8_t w;
        portENTER_CRITICAL(&g_dash_mux);
        w = g_dash.warn;
        portEXIT_CRITICAL(&g_dash_mux);

        if (w != 0 && prev == 0) {
            if (audio_alert_init() == ESP_OK) {
                play_beep_train(WARN_FREQ_HZ, WARN_BEEPS_PER_S, WARN_MAX_MS, warn_cleared_cb);
            }
        }
        prev = w;
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

esp_err_t audio_alert_init(void)
{
    return codec_hw_init();
}

void audio_alert_play_boot_async(void)
{
    xTaskCreatePinnedToCore(boot_task, "boot_beep", 6144, NULL, 5, NULL, 0);
}

void audio_alert_warn_monitor_start(void)
{
    xTaskCreate(warn_monitor_task, "warn_beep", 4096, NULL, 4, NULL);
}
