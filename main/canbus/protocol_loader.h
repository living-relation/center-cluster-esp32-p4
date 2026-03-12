#ifndef PROTOCOL_LOADER_H
#define PROTOCOL_LOADER_H

#include <stdint.h>

#define MAX_PROTOCOLS 8
#define MAX_FRAMES 32
#define MAX_SIGNALS 16
#define CAN_ID_MAX 2048

typedef enum{
    ENDIAN_BIG = 0,
    ENDIAN_LITTLE
} endian_t;

typedef enum{
    SIG_UNKNOWN = 0,
    SIG_RPM,
    SIG_SPEED,
    SIG_COOLANT_TEMP,
    SIG_AIR_TEMP,
    SIG_BATTERY_VOLTAGE,
    SIG_OIL_PRESSURE
} signal_id_t;

typedef struct{
    uint8_t offset;
    uint8_t len;
    float scale;
    float offset_val;
    endian_t endian;

    float *target;   // pointer to variable to update

} can_signal_t;

typedef struct{
    uint32_t id;
    uint8_t signal_count;
    can_signal_t signals[MAX_SIGNALS];
} can_frame_def_t;

typedef struct{
    char name[32];
    uint32_t bitrate;
    uint8_t frame_count;
    can_frame_def_t frames[MAX_FRAMES];
} can_protocol_t;

extern can_protocol_t protocols[MAX_PROTOCOLS];
extern int protocol_count;

extern can_protocol_t *active_protocol;

extern can_frame_def_t *frame_lookup[CAN_ID_MAX];

void protocol_loader_init(void);
void protocol_detect(uint32_t id);

#endif