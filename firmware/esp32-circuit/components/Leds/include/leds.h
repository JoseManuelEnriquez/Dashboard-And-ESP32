#ifndef LEDS_H
#define LEDS_H
#include "gpio.h"
#include "driver/gpio.h"
#include "board_definition.h"

typedef enum{
    LED_OK,
    LED_ERR_INVALID
}led_err_t;

led_err_t led_init();
void led_on(uint32_t pin);
void led_off(uint32_t pin);

#endif