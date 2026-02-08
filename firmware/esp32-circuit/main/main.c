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
#include "leds.h"
#include "communications.h"
#include "events.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
 Son variables definidas como extern en el main.h para que el modulo Events pueda comunicarse con
el main.
*/
volatile QueueHandle_t queue_event_mqtt = NULL;
volatile int wifi_connected = 0;

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

void vEventMQTT_Task(void* pvParameters){
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
                    const char* json_str = message.data;
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
 *                      MAIN
 * -------------------------------------------------
 */
void app_main(void)
{
    // ----- Configuration ----- // 

    /* 
    Inicializamos la cola necesaria para que la funcion callback definida en events.c pueda despertar
    la tarea vEventMQTT_Task y recoger los datos provenientes de la subscripcion.
    */
    queue_event_mqtt = xQueueCreate(10, sizeof(mqtt_message_t));
    led_err_t err_led = led_init();
    Button_err_t err_button = buttons_init(callback_buttons);
    wifi_init_sta(callback_init_wifi);
    
    /*
    Esperamos hasta que la funcion callback definida en events.h, cambie la variable a 1, 
    indicando que la conexion al punto de acceso ha sido realizada. 
    
    Debemos de esperar que tengamos conexion al punto de acceso porque sino no podremos 
    conectarnos al broker. Es decir, sin cumplir este paso no tendria sentido continuar con 
    la ejecucion del main y por eso hacemos polling.
    */

    led_on(CONFIGURATION_LED);
    led_off(CONNECTED_LED);
    while(wifi_connected == 0);

    led_on(CONFIGURATION_LED);
    led_off(CONNECTED_LED);

    mqtt_app_start();

    /*
    !! OJO !! Pongo por defecto 4096 pero habria que optimizar el valor para no desperdiciar memoria.
    CAMBIAR EN EL FUTURO
    */
    xTaskCreate(vControlFSMTask, "Control estados FSM", 4096, NULL, 6, NULL);
    xTaskCreate(vEventMQTT_Task, "Task para los eventos MQTT", 4096, NULL, 7, NULL);
}



