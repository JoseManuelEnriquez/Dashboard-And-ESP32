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

esp_err_t ldr_config(){
    gpio_config_t ldr_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << LDR_SENSOR),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    return gpio_config(&ldr_config);
}

esp_err_t leds_config()
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

esp_err_t button_config()
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

void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green)
{
    gpio_set_level(LED_RED, level_red);
    gpio_set_level(LED_YELLOW, level_yellow);
    gpio_set_level(LED_GREEN, level_green);
}


