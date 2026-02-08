#ifndef EVENTS_H
#define EVENTS_H

#include "board_definition.h"
#include "communications.h" 

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
}State_t;

typedef struct{
    QueueHandle_t queue_event_mqtt;
    int wifi_connected;
    State_t currentState;
}gEventStruct;

void events_init();
void callback_buttons(uint32_t io_num);
void callback_init_wifi(int conectado);
void callback_event_mqtt(mqtt_message_t message);
const gEventStruct* get_control_variables();

#endif