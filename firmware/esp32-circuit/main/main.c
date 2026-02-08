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
#include "main.h"
#include "DHT11.h"
#include "wifi.h"
#include "communications.h"
#include "events.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void vControlFSMTask(void* pvParameters)
{
    State_t previousState = -1;

    for(;;){
        if(currentState != previousState){
            switch (currentState)
            {
                case performance:
                // Encender lectura de sensores
                    break;
                case configuration:
                // Apagar lecturas de sensores
                    break;
                case idle:
                // Apagar lecturas de sensores
                    break;
                default:
                    break;
            }
            previousState = currentState;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void vEventMQTT(void* pvParameters){
    mqtt_message_t message;

    for(;;){
        xQueueReceive(queue_event_mqtt, &message, portMAX_DELAY);
        switch (message.topic)
        {
            case MQTT_ON:
                currentState = performance;
            break;
            case MQTT_SLEEP:
                currentState = idle;
            break;
            case MQTT_CONFIG:
                currentState = configuration;
            break;
            case MQTT_DELAY:
                if(currentState == configuration){
                    const char* json_str = event->data;
                    int delay_receive;
                    int result = json_scanf(json_str, strlen(json_str), "{delay: %d}", &delay_receive);
                    if(result){
                        // Llamar a la funcion del modulo sensor para set el delay.
                    }else{
                        // Publicar error
                    }
                }else{
                    // Publicar error
                }
            break;
            default:
            break;
        }
    }
}

/**
 * -------------------------------------------------
 * MAIN
 * -------------------------------------------------
 */
void app_main(void)
{
    queue_event_mqtt = xQueueCreate(10, sizeof(mqtt_message_t));
}



