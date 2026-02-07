#include "gpio.h"

const char* TAG_SENSOR = "SENSOR";
const char* TAG_CONFIG = "CONFIG";

QueueHandle_t isr_handler_queue; // Para despertar la tarea ChangeState

/**
 * -------------------------------------------------
 * FUNCIONES INTERRUPCIONES
 * -------------------------------------------------
 */
void IRAM_ATTR gpio_isr_change_button_handler(void* args) // Manejador de interrupcion
{   
    uint32_t io_num = CHANGE_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL);
}

void IRAM_ATTR gpio_isr_off_button_handler(void* args){
    uint32_t io_num = OFF_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL); 
}  

esp_err_t gpio_init(uint32_t intr_type, uint32_t mode, uint32_t bit_mask, uint32_t pull_down, uint32_t pull_up){
    gpio_config_t io_config = {
        .intr_type = intr_type,
        .mode = mode,
        .pin_bit_mask = bit_mask,
        .pull_down_en = pull_down,
        .pull_up_en = pull_up
    };
    return gpio_config(&ldr_config);
}

esp_err_t gpio_config_intr(uint32_t pin, ISR isr){
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(pin, isr, NULL);
}


