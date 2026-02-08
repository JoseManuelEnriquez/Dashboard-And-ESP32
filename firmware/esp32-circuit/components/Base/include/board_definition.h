#ifndef BOARD_DEFINITION_H
#define BOARD_DEFINITION_H

#include "driver/gpio.h"

#define LED_RED GPIO_NUM_23
#define LED_GREEN GPIO_NUM_21
#define LED_YELLOW GPIO_NUM_22
#define CONNECTED_LED GPIO_NUM_17
#define CONFIGURATION_LED GPIO_NUM_25
#define CHANGE_BUTTON GPIO_NUM_26
#define OFF_BUTTON GPIO_NUM_27
#define DHT11_SENSOR GPIO_NUM_14
#define LDR_SENSOR GPIO_NUM_19
#define ID 1  

/*
Hay que definir un delay minimo para la tarea ReadSensor porque el sensor DHT11 no puede hacer dos lecturas consecutivas
sin haber pasado mas de 2 segundos. Como el delay se puede configurar, hay que controlar que no configuren un delay menor 
a 2 segundos
*/
#define MIN_DELAY 2000

#endif