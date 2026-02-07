/**
 * @file main.c
 * @author Jose Manuel Enriquez Baena (joseenriquezbaena@gmail.com)
 * @brief Lectura de sensores y publicacion por MQTT
 * @version 1.5
 * @date 20-01-2026
 * @copyright Copyright (c) 2026
 * */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "DHT11.h"
#include "wifi.h"
#include "communications.h"
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * -----------------------------------------------------------
 * DEFINICIONES MACROS | VARIABLES GLOBALES | TIPOS DE DATOS
 * -----------------------------------------------------------
 */

#define ID 1  

#define LED_RED GPIO_NUM_23
#define LED_GREEN GPIO_NUM_21
#define LED_YELLOW GPIO_NUM_22
#define CONNECTED_LED GPIO_NUM_17
#define CONFIGURATION_LED GPIO_NUM_25
#define CHANGE_BUTTON GPIO_NUM_26
#define OFF_BUTTON GPIO_NUM_27
#define DHT11_SENSOR GPIO_NUM_14
#define LDR_SENSOR GPIO_NUM_19

/*
Hay que definir un delay minimo para la tarea ReadSensor porque el sensor DHT11 no puede hacer dos lecturas consecutivas
sin haber pasado mas de 2 segundos. Como el delay se puede configurar, hay que controlar que no configuren un delay menor 
a 2 segundos
*/
#define MIN_DELAY 2000

#define ESP_INTR_FLAG_DEFAULT 0

#define LOW 0
#define HIGH 1

/**
 *  @brief Estados principales del dispositivo
 *  
 *  - Performance: Lectura periodica de los sensores y envio al broker MQTT
 *  - configuration:
 *  - off: No se realiza lecturas del sensor el dispositivo (modo sleep)
 */
typedef enum
{
    performance,
    configuration,
    idle
} State_t;

static QueueHandle_t isr_handler_queue = NULL; // Para despertar la tarea ChangeState
static State_t currentState = idle; 

// Etiquetas para la funcion ESP_LOG
static const char* TAG_SENSOR = "SENSOR_TASK";
static const char* TAG_CONFIG = "CONFIG";

/**
 * -------------------------------------------------
 * PROTOTIPO DE FUNCIONES
 * -------------------------------------------------
 */

static void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green);
static esp_err_t button_config();
static esp_err_t leds_config();
static esp_err_t ldr_config();

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
        case idle:
            set_io_level(HIGH, LOW, LOW); // RED = HIGH, YELLOW = OFF, GREEN = LOW
            break;
        default:
            currentState = idle;
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
                data.light = gpio_get_level(LDR_SENSOR);
                data.humicity = humicity_int;
                data.temperature = temperature_int;
                //if(mqtt_connected == 1)
                publish_data(&data);                
            }else{
                switch (err)
                {
                case ESP_ERR_INVALID_ARG: ESP_LOGE(TAG_SENSOR, "ESP_ERR_INVALID_ARG");
                break;
                case ESP_ERR_INVALID_CRC: ESP_LOGE(TAG_SENSOR, "ESP_ERR_INVALID_CRC");
                break;
                case ESP_ERR_TIMEOUT: ESP_LOGE(TAG_SENSOR, "ESP_ERR_TIMEOUT");
                break;
                default:
                    break;
                }
            }
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
    ESP_ERROR_CHECK(leds_config());
    ESP_ERROR_CHECK(ldr_config());
    ESP_ERROR_CHECK(button_config());
    ESP_ERROR_CHECK(dht11_init(DHT11_SENSOR));
    ESP_LOGI(TAG_CONFIG, "HARDWARE INIT SUCCESS\n");
    isr_handler_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_set_level(CONFIGURATION_LED, HIGH);
    gpio_set_level(CONNECTED_LED, LOW);
    wifi_init_sta();
    ESP_LOGI(TAG_WIFI, "WIFI INIT SUCCESS");
    
    mqtt_app_start();
    // ------ CREATION TASKS ------

    // Gestiona los estados de la FSM
    xTaskCreate(vControlFSMTask,"FSM Control Task", 2048, NULL, 6, NULL); 
    
    // Lee los sensores en modo performance.
    xTaskCreate(vReadSensorTask,"Read Sensor Task", 4096, NULL, 6, NULL); 
    
    // Despertada por la ISR para cambiar de estado. Prioridad maxima para respuesta inmediata
    xTaskCreate(vChangeStateTask,"Change State Task", 4096, NULL, 7, NULL);
    
    // Tarea necesaria para hacer debug de los estados y caracteristicas de todas las tareas del sistema
    // xTaskCreate(vMonitorTask,"Monitor Task",4096, NULL, , NULL);
}

/**
 * -------------------------------------------------
 * DEFINICION DE FUNCIONES
 * -------------------------------------------------
 */

static esp_err_t ldr_config(){
    gpio_config_t ldr_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << LDR_SENSOR),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    return gpio_config(&ldr_config);
}

static esp_err_t leds_config()
{
    // Configuracion de pines
    gpio_config_t out_pin = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << LED_RED | 1ULL << LED_GREEN | 1ULL << LED_YELLOW | 1ULL << CONFIGURATION_LED | 1ULL << CONNECTED_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    return gpio_config(&out_pin);
}

static esp_err_t button_config()
{
    // Configuracion de pines
    esp_err_t err;
    gpio_config_t input_pin = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << CHANGE_BUTTON | 1ULL << OFF_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    err = gpio_config(&input_pin);

    // Configuracion de interrupciones
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(CHANGE_BUTTON, gpio_isr_change_button_handler, NULL);
    gpio_isr_handler_add(OFF_BUTTON, gpio_isr_off_button_handler, NULL);

    return err;
}

static void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green)
{
    gpio_set_level(LED_RED, level_red);
    gpio_set_level(LED_YELLOW, level_yellow);
    gpio_set_level(LED_GREEN, level_green);
}





