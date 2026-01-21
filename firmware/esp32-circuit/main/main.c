/**
 * @file main.c
 * @author Jose Manuel Enriquez Baena (joseenriquezbaena@gmail.com)
 * @brief Lectura de sensores y publicacion por MQTT
 * @version 1.0
 * @date 20-01-2026
 * * @copyright Copyright (c) 2026
 * */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * -------------------------------------------------
 * DEFINICIONES MACROS VARIABLES GLOBALES
 * -------------------------------------------------
 */

#define LED_TEST GPIO_NUM_32
#define LED_RED 1
#define LED_GREEN 2
#define LED_YELLOW 3
#define CHANGE_BUTTON 1
#define OFF_BUTTON 2
#define TEST_BUTTON GPIO_NUM_26
#define ESP_INTR_FLAG_DEFAULT 0

#define LOW 0
#define HIGH 1

typedef enum
{
    performance,
    configuration,
    off
} State_t;

State_t currentState = off;                 // variable global que controla en que modo trabaja el circuito

/**
 * -------------------------------------------------
 * PROTOTIPO DE FUNCIONES
 * -------------------------------------------------
 */
void debug_io(uint64_t io);
uint32_t button_pressed(uint32_t button);
void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green);
void button_config();
void leds_config();

/**
 * -------------------------------------------------
 * FUNCIONES INTERRUPCIONES
 * -------------------------------------------------
 */
static void IRAM_ATTR gpio_isr_button_handler(void* args) // Manejador de interrupcion
{   
    uint32_t io_num = (uint32_t) args;
    currentState = performance;
}

/**
 * -------------------------------------------------
 * MAIN
 * -------------------------------------------------
 */
void app_main(void)
{
    // ------ CONFIGURATION ------
    leds_config();
    button_config();

    // ------ SUPERLOOP ------
    while (1)
    {
        switch (currentState)
        {
        case performance:
            //set_io_level(LOW, LOW, HIGH); // RED = OFF, YELLOW = OFF, GREEN = HIGH
            gpio_set_level(LED_TEST, HIGH);
            break;
        case configuration:
            //set_io_level(LOW, HIGH, LOW); // RED = OFF, YELLOW = HIGH, GREEN = LOW
            gpio_set_level(LED_TEST, LOW);
            break;
        case off:
            //set_io_level(HIGH, LOW, LOW); // RED = HIGH, YELLOW = OFF, GREEN = LOW
            break;
        default:
            currentState = off;
            break;
        }
    }
}

/**
 * -------------------------------------------------
 * DEFINICION DE FUNCIONES
 * -------------------------------------------------
 */
void debug_io(uint64_t io)
{
    gpio_dump_io_configuration(stdout, io);
}

void leds_config(){
    // Configuracion de pines
    gpio_config_t out_pin = {};
    out_pin.intr_type = GPIO_INTR_DISABLE;
    out_pin.pin_bit_mask = (1ULL << LED_TEST); //| 1ULL << LED_RED | 1ULL << LED_GREEN | 1ULL << LED_YELLOW);
    out_pin.mode = GPIO_MODE_OUTPUT;
    out_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    out_pin.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&out_pin);
}

void button_config(){
    // Configuracion de pines
    gpio_config_t input_pin = {};
    input_pin.intr_type = GPIO_INTR_NEGEDGE;
    input_pin.pin_bit_mask = (1ULL << CHANGE_BUTTON | 1ULL << OFF_BUTTON | 1ULL << TEST_BUTTON);
    input_pin.mode = GPIO_MODE_INPUT;
    input_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_pin.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&input_pin);

    // Configuracion de interrupciones
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(CHANGE_BUTTON, gpio_isr_button_handler, (void *)CHANGE_BUTTON);
    gpio_isr_handler_add(OFF_BUTTON, gpio_isr_button_handler, (void *)OFF_BUTTON);
}

uint32_t button_pressed(uint32_t button)
{
    return gpio_get_level(button);
}

void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green)
{
    gpio_set_level(LED_RED, level_red);
    gpio_set_level(LED_YELLOW, level_yellow);
    gpio_set_level(LED_GREEN, level_green);
}