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
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
Hay que definir un delay minimo para la tarea ReadSensor porque el sensor DHT11 no puede hacer dos lecturas consecutivas
sin haber pasado mas de 2 segundos. Como el delay se puede configurar, hay que controlar que no configuren un delay menor 
a 2 segundos
*/
#define MIN_DELAY 2000

void callback_buttons(uint32_T io_num){
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

volatile static State_t currentState = idle; 

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

/*
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
*/
 
/**
 * -------------------------------------------------
 * MAIN
 * -------------------------------------------------
 */
void app_main(void)
{
    // ------ CONFIGURATION ------
    // ESP_LOGI(TAG_CONFIG, "HARDWARE INIT SUCCESS\n");
    //wifi_init_sta();
    //ESP_LOGI(TAG_WIFI, "WIFI INIT SUCCESS");
    
    //mqtt_app_start();
    // ------ CREATION TASKS ------

    // Gestiona los estados de la FSM
    //xTaskCreate(vControlFSMTask,"FSM Control Task", 2048, NULL, 6, NULL); 
    
    // Lee los sensores en modo performance.
    //xTaskCreate(vReadSensorTask,"Read Sensor Task", 4096, NULL, 6, NULL); 
}



