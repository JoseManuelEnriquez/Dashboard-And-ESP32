/**
 * @file main.c
 * @author Jose Manuel Enriquez Baena (joseenriquezbaena@gmail.com)
 * @brief Lectura de sensores y publicacion por MQTT
 * @version 1.2
 * @date 20-01-2026
 * * @copyright Copyright (c) 2026
 * */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "DHT11.h"
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
#define DHT11_SENSOR GPIO_NUM_14
#define ESP_INTR_FLAG_DEFAULT 0

#define LOW 0
#define HIGH 1

typedef enum
{
    performance,
    configuration,
    off
} State_t;

typedef struct{
    uint8_t humicity;
    uint8_t temperature;
    uint8_t light;
}data_t;

static QueueHandle_t isr_handler_queue = NULL;
static State_t currentState = performance;
static const char* TAG_SENSOR = "SENSOR_TASK";

/**
 * -------------------------------------------------
 * PROTOTIPO DE FUNCIONES
 * -------------------------------------------------
 */
void debug_io(uint64_t io);
void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green);
esp_err_t button_config();
esp_err_t leds_config();

/**
 * -------------------------------------------------
 * FUNCIONES DE TAREAS freeRTOS
 * -------------------------------------------------
 */

void vMonitorTask(void *pvParameters) {
    char buffer[400];
    for (;;) {
        printf("\n--- Task List ---\n");
        printf("Nombre          Estado  Prio  Stack   ID\n");
        vTaskList(buffer);
        printf("%s", buffer);
        printf("-----------------\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void vControlFSMTask(void* pvParameters)
{
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
        vTaskDelay(pdMS_TO_TICKS(100));
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
                }else if(currentState == configuration){
                    currentState = performance;
                }else{
                    currentState = performance;
                }
            }
        }
    }
    vTaskDelete(NULL);
}

void vReadSensorTask(void* pvParameters)
{
    data_t data;
    uint8_t humicity_int, humicity_dec, temperature_int, temperature_dec;
    esp_err_t err;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;){
        switch(currentState){
            case performance:
            err = dht11_read(DHT11_SENSOR, &humicity_int, &humicity_dec, &temperature_int, &temperature_dec);
            if(err == ESP_OK){
                data.humicity = humicity_int;
                data.temperature = temperature_int;
                ESP_LOGI(TAG_SENSOR, "Lectura de humedad: %d | Lectura de temperatura: %d", data.humicity, data.temperature);
                // Despertar la tarea de mandar datos por MQTT mandando la estructura data 
            }else{
                switch (err)
                {
                case ESP_ERR_INVALID_ARG: ESP_LOGE(TAG_SENSOR, "Argumentos invalidos\n");
                break;
                case ESP_ERR_INVALID_CRC: ESP_LOGE(TAG_SENSOR, "Error de checksum\n");
                break;
                case ESP_ERR_TIMEOUT: ESP_LOGE(TAG_SENSOR, "Error de timeout \n");
                break;
                default:
                    break;
                }
            }
            break;
            case configuration:
            break;
            case off:
            break;
            default:
            break;
        }
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL);
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
    dht11_init(DHT11_SENSOR);
    isr_handler_queue = xQueueCreate(10, sizeof(uint32_t)); // Crear cola para manejar interrupcion

    // ------ CREATION TASKS ------
    xTaskCreate(vControlFSMTask,"FSM Control Task", 2048, NULL, 6, NULL); // Tarea que controla los estados del fsm
    xTaskCreate(vReadSensorTask,"Read Sensor Task", 4096, NULL, 6, NULL); // Tarea que dependiendo del modo, lee el sensor o no
    xTaskCreate(vChangeStateTask,"Change State Task", 4096, NULL, 7, NULL); // Tarea que la isr despierta para cambiar de estado
    // xTaskCreate(vMonitorTask,"Monitor Task",4096, NULL, , NULL);// Tarea para DEBUG
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
    input_pin.intr_type = GPIO_INTR_NEGEDGE;
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

void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green)
{
    gpio_set_level(LED_RED, level_red);
    gpio_set_level(LED_YELLOW, level_yellow);
    gpio_set_level(LED_GREEN, level_green);
}