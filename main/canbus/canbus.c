#include "canbus.h"
#include "protocol_loader.h"

#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "CANBUS";


// =======================================================
// GPIO CONFIG
// =======================================================

#define CAN_DEFAULT_BITRATE 1000000
#define CAN_TX GPIO_NUM_5
#define CAN_RX GPIO_NUM_4

// =======================================================
// GLOBAL DATA
// =======================================================

can_dash_data_t can_data = {0};


// =======================================================
// CAN FRAME PROCESSOR
// =======================================================

void process_can_frame(uint32_t id, uint8_t *data)
{
    protocol_detect(id);

    if (!active_protocol)
        return;

    can_frame_def_t *frame = NULL;

    if (id < CAN_ID_MAX)
        frame = frame_lookup[id];

    if (!frame)
        return;

    for (int s = 0; s < frame->signal_count; s++){
        can_signal_t *sig = &frame->signals[s];

        uint32_t raw = 0;

        if (sig->len == 2){
            if (sig->endian == ENDIAN_BIG)
                raw = (data[sig->offset] << 8) | data[sig->offset+1];
            else
                raw = (data[sig->offset+1] << 8) | data[sig->offset];
        }
        else if (sig->len == 1){
            raw = data[sig->offset];
        }

        if (sig->target){
            *sig->target = raw * sig->scale + sig->offset_val;
        }
    }
}


// =======================================================
// CAN INIT
// =======================================================

void canbus_init(void){
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);

    #if CAN_DEFAULT_BITRATE == 1000000
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    #elif CAN_DEFAULT_BITRATE == 500000
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    #elif CAN_DEFAULT_BITRATE == 250000
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    #else
    #error Unsupported CAN bitrate
    #endif

        twai_filter_config_t f_config =
            TWAI_FILTER_CONFIG_ACCEPT_ALL();

        ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
        ESP_ERROR_CHECK(twai_start());

        protocol_loader_init();

        ESP_LOGI(TAG, "CAN initialized");
}


// =======================================================
// CAN RECEIVE TASK
// =======================================================

void canbus_task(void *arg)
{
    twai_message_t message;

    while (1)
    {
        if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK)
        {
            process_can_frame(message.identifier, message.data);
        }
    }
}