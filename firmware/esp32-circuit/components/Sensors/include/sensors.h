#ifndef SENSORS_H
#define SENSORS_H

#include "board_definition.h"

typedef struct{
    uint8_t light;
    uint8_t temperature;
    uint8_t humidicity;
}sensor_data_t;

typedef enum{
    SENSOR_OK,
    SENSOR_ERR_INVALID
}eSensor_error;

void sensors_init();
void sensors_on();
void sensors_off();
eSensor_error readSensors(sensor_data_t* data);


#endif