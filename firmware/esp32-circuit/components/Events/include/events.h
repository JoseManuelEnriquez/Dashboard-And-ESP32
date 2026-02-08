#ifndef EVENTS_H
#define EVENTS_H
#include <cstdint>
#include "board_definition.h"
#include "communications.h" 
#include "main.h"

void callback_buttons(uint32_t io_num);
void callback_init_wifi(int conectado);
void callback_event_mqtt(mqtt_message_t message);

#endif