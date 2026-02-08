#ifndef MAIN_H
#define MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 *  @brief Estados principales del dispositivo
 *  
 *  - Performance: Lectura periodica de los sensores y envio al broker MQTT
 *  - configuration:
 *  - idle: No se realiza lecturas del sensor el dispositivo (modo sleep)
 */
typedef enum
{
    performance,
    configuration,
    idle
} State_t;


extern volatile State_t currentState;
extern volatile int wifi_connected; 
extern volatile QueueHandle_t queue_event_mqtt;

#endif