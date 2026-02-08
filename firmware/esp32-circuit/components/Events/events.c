#include "events.h"

void callback_buttons(uint32_t io_num){
    if(io_num == OFF_BUTTON){
        currentState = idle;
    }else{
        if(currentState == performance){
            currentState = configuration;
        }else if(currentState == configuration){
            currentState = performance;
        }else{
            currentState = performance;
        }
    }
}

void callback_init_wifi(int conectado){
    if(conectado == 1){
        wifi_connected = 1;
    }else{
        wifi_connected = 0;
    }
}

void callback_event_mqtt(mqtt_message_t message){
    xQueueSend(queue_event_mqtt, &message, 0);
}

