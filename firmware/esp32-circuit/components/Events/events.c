#include "events.h"

static gEventStruct gControlVariables;

void events_init(){
    gControlVariables.wifi_connected = 0;
    gControlVariables.currentState = idle;
    gControlVariables.queue_event_mqtt = xQueueCreate(10, sizeof(mqtt_message_t));
}

void callback_buttons(uint32_t io_num){
    if(io_num == OFF_BUTTON){
        gControlVariables.currentState = idle;
    }else{
        if(gControlVariables.currentState == performance){
            gControlVariables.currentState = configuration;
        }else if(gControlVariables.currentState == configuration){
            gControlVariables.currentState = performance;
        }else{
            gControlVariables.currentState = performance;
        }
    }
}

void callback_init_wifi(int conectado){
    if(conectado == 1){
        gControlVariables.wifi_connected = 1;
    }else{
        gControlVariables.wifi_connected = 0;
    }
}

void callback_event_mqtt(mqtt_message_t message){
    xQueueSend(gControlVariables.queue_event_mqtt, &message, 0);
}

const gEventStruct* get_control_variables(){
    return &gControlVariables;
}