#include "canbus.h"
#include "protocol_loader.h"

#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <string.h>

#include "esp_spiffs.h"

static const char *TAG = "CANBUS";

static const int can_rates[] = {
    1000000,
    500000,
    250000,
    125000
};

#define NUM_RATES (sizeof(can_rates)/sizeof(can_rates[0]))


// =======================================================
// GPIO CONFIG
// =======================================================

#define CAN_TX GPIO_NUM_5
#define CAN_RX GPIO_NUM_4

// =======================================================
// GLOBAL DATA
// =======================================================

volatile can_dash_data_t can_data = {0};


// =======================================================
// CAN FRAME PROCESSOR
// =======================================================

void process_can_frame(uint32_t id, uint8_t *data){
    protocol_detect(id);

    if(!active_protocol)
        return;

    if(id >= CAN_ID_MAX)
        return;

    can_frame_def_t *frame = frame_lookup[id];

    if(!frame)
        return;

    for(int s=0;s<frame->signal_count;s++){
        can_signal_t *sig = &frame->signals[s];

        uint32_t raw = 0;

        if(sig->len==2){
            if(sig->endian==ENDIAN_BIG)
                raw=(data[sig->offset]<<8)|data[sig->offset+1];
            else
                raw=(data[sig->offset+1]<<8)|data[sig->offset];
        }
        else{
            raw=data[sig->offset];
        }

        if(sig->target)
            *sig->target = raw*sig->scale + sig->offset_val;
    }
}


void mount_fs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

static twai_timing_config_t get_timing(int bitrate){
    twai_timing_config_t t;

    switch (bitrate){
        case 1000000:
            t = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
            break;

        case 500000:
            t = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
            break;

        case 250000:
            t = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
            break;

        case 125000:
            t = (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS();
            break;

        default:
            t = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
            break;
    }

    return t;
}


int detect_can_bitrate()
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);

    twai_filter_config_t f_config =
        TWAI_FILTER_CONFIG_ACCEPT_ALL();

    twai_message_t msg;

    for (int i = 0; i < NUM_RATES; i++){
        int rate = can_rates[i];

        twai_timing_config_t t_config = get_timing(rate);

        ESP_LOGI(TAG, "Trying CAN bitrate %d", rate);

        ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
        ESP_ERROR_CHECK(twai_start());

        int frames = 0;
        int timeout = 20;

        while (timeout--){
            if (twai_receive(&msg, pdMS_TO_TICKS(10)) == ESP_OK){
                if (msg.extd || msg.rtr)
                    continue;

                frames++;

                if (frames >= 3){
                    ESP_LOGI(TAG, "Detected CAN bitrate %d", rate);

                    twai_stop();
                    twai_driver_uninstall();

                    return rate;
                }
            }
        }

        twai_stop();
        twai_driver_uninstall();
    }

    ESP_LOGW(TAG, "CAN bitrate detection failed, defaulting to 500k");

    return 500000;
}


// =======================================================
// CAN INIT
// =======================================================

void canbus_init(void)
{
    mount_fs();

    protocol_loader_init();

    int bitrate = detect_can_bitrate();

    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);

    twai_timing_config_t t_config = get_timing(bitrate);

    twai_filter_config_t f_config =
        TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());

    ESP_LOGI(TAG, "CAN initialized at %d", bitrate);
}


// =======================================================
// CAN RECEIVE TASK
// =======================================================

void canbus_task(void *arg){
    twai_message_t message;

    while (1){
        if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK){
            if (!message.extd && !message.rtr)
            process_can_frame(message.identifier, message.data);
        }
    }
}