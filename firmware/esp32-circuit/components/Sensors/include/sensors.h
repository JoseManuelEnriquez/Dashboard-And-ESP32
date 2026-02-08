#ifndef SENSORS_H
#define SENSORS_H

#include "board_definition.h"
#include "gpio.h"
#include "DHT11.h" 

typedef struct{
    uint8_t light;
    uint8_t temperature;
    uint8_t humidicity;
}sensor_data_t;

typedef enum{
    SENSOR_OK,
    SENSOR_ERR_INVALID,
    SENSOR_ERR_STATE,
    SENSOR_ERR_READ
}eSensor_error;

eSensor_error sensors_init();
void sensors_on();
void sensors_off();
eSensor_error readSensors(sensor_data_t* data);


#endif