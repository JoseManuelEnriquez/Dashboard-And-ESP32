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

#define LED_RED GPIO_NUM_25
#define LED_GREEN GPIO_NUM_32
#define LED_YELLOW GPIO_NUM_33
#define CHANGE_BUTTON GPIO_NUM_26
#define OFF_BUTTON GPIO_NUM_27
#define ESP_INTR_FLAG_DEFAULT 0

#define LOW 0
#define HIGH 1

typedef enum
{
    performance,
    configuration,
    off
} State_t;

static QueueHandle_t isr_handler_queue = NULL;
static State_t currentState = off;

/**
 * -------------------------------------------------
 * PROTOTIPO DE FUNCIONES
 * -------------------------------------------------
 */
void debug_io(uint64_t io);
uint32_t button_pressed(uint32_t button);
void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green);
esp_err_t button_config();
esp_err_t leds_config();

/**
 * -------------------------------------------------
 * FUNCIONES DE TAREAS freeRTOS
 * -------------------------------------------------
 */

void vControl_FSMTask(void* pvParameters){
    for(;;){
        switch (currentState)
        {
        case performance:
            set_io_level(LOW, LOW, HIGH); // RED = OFF, YELLOW = OFF, GREEN = HIGH
            break;
        case configuration:
            set_io_level(LOW, HIGH, LOW); // RED = OFF, YELLOW = HIGH, GREEN = LOW
            break;
        case off:
            set_io_level(HIGH, LOW, LOW); // RED = HIGH, YELLOW = OFF, GREEN = LOW
            break;
        default:
            currentState = off;
            break;
        }
        vTaskDelay(3000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void vChangeStateTask(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(isr_handler_queue, &io_num, portMAX_DELAY)) {
            if(io_num == OFF_BUTTON){
                currentState = off;
            }else{
                if(currentState == performance){
                    currentState = configuration;
                }else{
                    currentState = performance;
                }
            }
        }
    }
}



/**
 * -------------------------------------------------
 * FUNCIONES INTERRUPCIONES
 * -------------------------------------------------
 */
static void IRAM_ATTR gpio_isr_change_button_handler(void* args) // Manejador de interrupcion
{   
    uint32_t io_num = CHANGE_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL);
}

static void IRAM_ATTR gpio_isr_off_button_handler(void* args){
    uint32_t io_num = OFF_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL); 
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
    isr_handler_queue = xQueueCreate(10, sizeof(uint32_t)); // Crear cola para manejar interrupcion

    // ------ CREATION TASKS ------
    xTaskCreate(vControl_FSMTask,"FSM Control Task", 512, NULL, 6, NULL);
    xTaskCreate(vChangeStateTask,"Change StateTask", 512, NULL, 7, NULL); // Creo la tarea para manejar interrupcione

    for(;;);
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

esp_err_t leds_config()
{
    // Configuracion de pines
    gpio_config_t out_pin = {};
    out_pin.intr_type = GPIO_INTR_DISABLE;
    out_pin.pin_bit_mask = (1ULL << LED_RED | 1ULL << LED_GREEN | 1ULL << LED_YELLOW);
    out_pin.mode = GPIO_MODE_OUTPUT;
    out_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    out_pin.pull_up_en = GPIO_PULLUP_DISABLE;
    return gpio_config(&out_pin);
}

esp_err_t button_config()
{
    // Configuracion de pines
    esp_err_t err;
    gpio_config_t input_pin = {};
    input_pin.intr_type = GPIO_INTR_LOW_LEVEL;
    input_pin.pin_bit_mask = (1ULL << CHANGE_BUTTON | 1ULL << OFF_BUTTON);
    input_pin.mode = GPIO_MODE_INPUT;
    input_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_pin.pull_up_en = GPIO_PULLUP_ENABLE;
    err = gpio_config(&input_pin);

    // Configuracion de interrupciones
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(CHANGE_BUTTON, gpio_isr_change_button_handler, NULL);
    gpio_isr_handler_add(OFF_BUTTON, gpio_isr_off_button_handler, NULL);

    return err;
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