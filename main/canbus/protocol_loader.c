#include "protocol_loader.h"
#include "protocol_list.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "PROTO";

can_frame_def_t *frame_lookup[CAN_ID_MAX];

static void (*decode_table[CAN_ID_MAX])(uint8_t *data);

can_protocol_t protocols[MAX_PROTOCOLS];
int protocol_count = 0;

can_protocol_t *active_protocol = NULL;

extern can_dash_data_t can_data;

static uint32_t seen_ids[64];
static int seen_count = 0;
static int64_t detect_start = 0;
static bool detection_done = false;


static float* signal_name_to_ptr(const char *name){
    if (!strcmp(name,"rpm")) return &can_data.rpm;
    if (!strcmp(name,"speed")) return &can_data.speed;
    if (!strcmp(name,"coolant_temp")) return &can_data.coolant_temp;
    if (!strcmp(name,"air_temp")) return &can_data.air_temp;
    if (!strcmp(name,"battery_voltage")) return &can_data.battery_voltage;
    if (!strcmp(name,"oil_pressure")) return &can_data.oil_pressure;

    return NULL;
}

static void record_id(uint32_t id){
    for(int i=0;i<seen_count;i++)
        if(seen_ids[i]==id)
            return;

    if(seen_count < 64)
        seen_ids[seen_count++] = id;
}

static void decode_generic(uint8_t *data){
    if(!active_protocol)
        return;

    can_frame_def_t *frame = NULL;

    uint32_t id = *((uint32_t*)data); // placeholder

    if(id < CAN_ID_MAX)
        frame = frame_lookup[id];

    if(!frame)
        return;

    for(int s=0;s<frame->signal_count;s++){
        can_signal_t *sig = &frame->signals[s];

        uint32_t raw = 0;

        if(sig->len == 2){
            if(sig->endian == ENDIAN_BIG)
                raw = (data[sig->offset]<<8) | data[sig->offset+1];
            else
                raw = (data[sig->offset+1]<<8) | data[sig->offset];
        }
        else
        {
            raw = data[sig->offset];
        }

        if(sig->target)
            *sig->target = raw * sig->scale + sig->offset_val;
    }
}

static void load_protocol_from_json(const char *json){
    cJSON *root = cJSON_Parse(json);

    if(!root)
        return;

    if(protocol_count >= MAX_PROTOCOLS){
        cJSON_Delete(root);
        return;
    }

    can_protocol_t *proto = &protocols[protocol_count];

    cJSON *name = cJSON_GetObjectItem(root,"name");
    cJSON *frames = cJSON_GetObjectItem(root,"frames");
    cJSON *bitrate = cJSON_GetObjectItem(root,"bitrate");

    if(name && name->valuestring){
        strncpy(proto->name,name->valuestring,sizeof(proto->name)-1);
        proto->name[sizeof(proto->name)-1] = 0;
    }

    if(bitrate)
        proto->bitrate = bitrate->valueint;

    int frame_count = cJSON_GetArraySize(frames);
    proto->frame_count = frame_count > MAX_FRAMES ? MAX_FRAMES : frame_count;

    for(int i=0;i<proto->frame_count;i++){
        cJSON *frame = cJSON_GetArrayItem(frames,i);

        cJSON *id = cJSON_GetObjectItem(frame,"id");
        cJSON *signals = cJSON_GetObjectItem(frame,"signals");

        if(cJSON_IsString(id))
            proto->frames[i].id = strtol(id->valuestring,NULL,0);
        else
            proto->frames[i].id = id->valueint;

        int sig_count = cJSON_GetArraySize(signals);
        proto->frames[i].signal_count = sig_count > MAX_SIGNALS ? MAX_SIGNALS : sig_count;

        for(int s=0;s<proto->frames[i].signal_count;s++){
            cJSON *sig = cJSON_GetArrayItem(signals,s);

            can_signal_t *signal = &proto->frames[i].signals[s];

            cJSON *name = cJSON_GetObjectItem(sig,"name");
            cJSON *offset = cJSON_GetObjectItem(sig,"offset");
            cJSON *len = cJSON_GetObjectItem(sig,"len");
            cJSON *scale = cJSON_GetObjectItem(sig,"scale");
            cJSON *offset_val = cJSON_GetObjectItem(sig,"offset_val");
            cJSON *endian = cJSON_GetObjectItem(sig,"endian");

            signal->target = name ? signal_name_to_ptr(name->valuestring) : NULL;
            signal->offset = offset ? offset->valueint : 0;
            signal->len = len ? len->valueint : 1;
            signal->scale = scale ? scale->valuedouble : 1.0f;
            signal->offset_val = offset_val ? offset_val->valuedouble : 0;

            if(endian && endian->valuestring && !strcmp(endian->valuestring,"little"))
                signal->endian = ENDIAN_LITTLE;
            else
                signal->endian = ENDIAN_BIG;
        }
    }

    protocol_count++;

    cJSON_Delete(root);
}

void protocol_loader_init(void){
    ESP_LOGI(TAG,"Loading CAN protocols");

    protocol_count = 0;
    active_protocol = NULL;

    memset(frame_lookup,0,sizeof(frame_lookup));
    memset(decode_table,0,sizeof(decode_table));

    for(int i=0;i<protocol_json_count;i++)
        load_protocol_from_json(protocol_json_list[i]);

    for(int p=0;p<protocol_count;p++){
        can_protocol_t *proto = &protocols[p];

        for(int f=0;f<proto->frame_count;f++){
            uint32_t id = proto->frames[f].id;

            if(id < CAN_ID_MAX){
                frame_lookup[id] = &proto->frames[f];
                decode_table[id] = decode_generic;
            }
        }
    }

    ESP_LOGI(TAG,"Loaded %d CAN protocols",protocol_count);
}

void protocol_detect(uint32_t id){
    if(active_protocol)
        return;

    record_id(id);

    if(detect_start==0)
        detect_start = esp_timer_get_time();

    int64_t now = esp_timer_get_time();

    if((now-detect_start) < 200000)
        return;

    for(int p=0;p<protocol_count;p++){
        int matches=0;

        for(int f=0;f<protocols[p].frame_count;f++){
            uint32_t frame_id = protocols[p].frames[f].id;

            for(int s=0;s<seen_count;s++){
                if(frame_id==seen_ids[s]){
                    matches++;
                    break;
                }
            }

            if(matches>=2){
                active_protocol=&protocols[p];
                detection_done=true;

                ESP_LOGI(TAG,"Detected CAN protocol: %s",active_protocol->name);
                return;
            }
        }
    }
}