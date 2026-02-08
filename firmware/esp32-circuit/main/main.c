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
#include "buttons.h"
#include "events.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
Son flags definidas como extern en el main.h para que el modulo Events pueda comunicarse con
el main.

Se definen en volatile por evitar errores de concurrencia.
*/
volatile QueueHandle_t queue_event_mqtt = NULL;
volatile int wifi_connected = 0;
volatile State_t currentState = idle;

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
    /* 
    La interfaz de los modulos estan diseñados con el paradigma de orientado a eventos, 
    respondiendo con funciones callbacks definidas en el modulo events.h que tiene metodos de sincronizacion
    necesarios para conseguir el maximo desacoplamiento.

    Los modulos que necesitan un callback son: 
    - BUTTONS: Necesario para las interrupciones cuando se pulsa el boton.
    - WIFI: para conocer si la conexion se ha realizado.
    - MQTT: para recibir los datos de los topicos en los que esta suscrito el ESP32.
    Por tanto, sus funciones init reciben una funcion segun esta establecido en su interfaz implementada.
    */

    // ----- Configuration ----- // 
    queue_event_mqtt = xQueueCreate(10, sizeof(mqtt_message_t));
    led_err_t err_led = led_init();
    Button_err_t err_button = buttons_init(callback_buttons);
    wifi_init_sta(callback_init_wifi);
    
        /* * BLOQUEO POR DEPENDENCIA DE RED
    * Se realiza un polling sobre la bandera actualizada por el callback de Wi-Fi (events.h).
    * Esta espera activa es necesaria ya que la conexión al Broker MQTT depende 
    * estrictamente de la obtención previa de una dirección IP. 
    * Se detiene el flujo del main para garantizar la integridad de la secuencia de red.
    */

    led_on(CONFIGURATION_LED);
    led_off(CONNECTED_LED);
    while(wifi_connected == 0);

    led_on(CONFIGURATION_LED);
    led_off(CONNECTED_LED);

    mqtt_app_start(callback_event_mqtt);

    /*
    !! OJO !! Pongo por defecto 4096 pero habria que optimizar el valor para no desperdiciar memoria.
    CAMBIAR EN EL FUTURO
    */
    xTaskCreate(vControlFSMTask, "Control estados FSM", 4096, NULL, 6, NULL);
    xTaskCreate(vEventMQTT_Task, "Task para los eventos MQTT", 4096, NULL, 7, NULL);
}



