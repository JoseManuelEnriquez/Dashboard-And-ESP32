#include <stdio.h>
#include "driver/gpio.h"

#define LED_TEST  GPIO_NUM_32
#define LED_RED 1
#define LED_GREEN 2
#define LED_YELLOW 3
#define CHANGE_BUTTON 1
#define OFF_BUTTON 2

#define LOW 0
#define HIGH 1

void debug_io(uint64_t io){
    gpio_dump_io_configuration(stdout, io);
}

void init_io(){
    gpio_config_t out_pin = {};
    gpio_config_t input_pin = {};

    out_pin.intr_type = GPIO_INTR_DISABLE;
    out_pin.pin_bit_mask = ( 1ULL << LED_TEST); //| 1ULL << LED_RED | 1ULL << LED_GREEN | 1ULL << LED_YELLOW);
    out_pin.mode = GPIO_MODE_OUTPUT;
    out_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    out_pin.pull_up_en = GPIO_PULLUP_DISABLE;
    
    input_pin.intr_type = GPIO_INTR_DISABLE;
    input_pin.pin_bit_mask = (1ULL << CHANGE_BUTTON | 1ULL << OFF_BUTTON);
    input_pin.mode = GPIO_MODE_INPUT;
    input_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_pin.pull_up_en = GPIO_PULLUP_DISABLE;

    gpio_config(&out_pin);
    gpio_config(&input_pin);
}

void app_main(void)
{
    // ------ CONFIGURATION ------
    init_io();
    
    // ------ SUPERLOOP ------
    while(1){
        gpio_set_level(LED_TEST, HIGH);
    }
}
