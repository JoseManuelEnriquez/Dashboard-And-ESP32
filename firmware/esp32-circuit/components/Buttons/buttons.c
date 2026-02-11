/**
 * @file buttons.c
 * @brief Implementación de la inicializacion, configuracion y uso de botones
 */

#include "buttons.h"

/**
 * Variables globales necesarias por las ISR y la tarea para manejar interrupciones.
 * La callback es llamada por la tarea cuando alguna ISR la despierte.
 * isr_handler_queue es la cola que bloqueara la tarea hasta que reciba algun dato.
 */
static button_callback callback_private = NULL; 
static QueueHandle_t isr_handler_queue; 

/**
 * -------------------------------------------------
 * FUNCIONES INTERRUPCIONES
 * -------------------------------------------------
 * 
 * Existen dos ISR, una para cada boton del sistema:
 * - CHANGE_BUTTON: Este boton cambia del estado performance al configuracion y viceversa.
 * - OFF_BUTTON: Este boton cambia de cualquier estado al idle.
 * 
 * La ISR manda a la cola que despierta la tarea el numero del pin del boton que genera la interrupcion
 */

void vButtonISRTask(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(isr_handler_queue, &io_num, portMAX_DELAY)) {
            if(callback_private != NULL)
                callback_private(io_num);
        }
    }
    vTaskDelete(NULL);
}

void IRAM_ATTR gpio_isr_change_button_handler(void* args)
{   
    uint32_t io_num = CHANGE_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL);
}

void IRAM_ATTR gpio_isr_off_button_handler(void* args){
    uint32_t io_num = OFF_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL); 
}  

Button_err_t buttons_init(button_callback callback){
    callback_private = callback;
    uint32_t bitmask = (1ULL << CHANGE_BUTTON | 1ULL << OFF_BUTTON);
    
    esp_err_t err_init = gpio_init(GPIO_INTR_LOW_LEVEL, GPIO_MODE_INPUT, bitmask, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_ENABLE);
    esp_err_t err_intr_change = gpio_config_intr(CHANGE_BUTTON, gpio_isr_change_button_handler);
    esp_err_t err_intr_off = gpio_config_intr(OFF_BUTTON, gpio_isr_off_button_handler);
    
    xTaskCreate(vButtonISRTask,"ButtonISR", 4096, NULL, 7, NULL);
    isr_handler_queue = xQueueCreate(10, sizeof(uint32_t));
    return (err_init == ESP_OK && err_intr_change == ESP_OK && err_intr_off == ESP_OK) ? BUTTON_OK : BUTTON_ERR_INVALID;
}



