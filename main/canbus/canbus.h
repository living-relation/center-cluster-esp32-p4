#ifndef CANBUS_H
#define CANBUS_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/twai.h"


// =======================================================
// DASH DATA STRUCTURE
// =======================================================

typedef struct{
    float rpm;
    float speed;
    float coolant_temp;
    float air_temp;
    float battery_voltage;
    float oil_pressure;
} can_dash_data_t;


// global decoded data
extern volatile can_dash_data_t can_data;



// =======================================================
// API
// =======================================================

void canbus_init(void);
void canbus_task(void *arg);
void process_can_frame(uint32_t id, uint8_t *data);

#endif