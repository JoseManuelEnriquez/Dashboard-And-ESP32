#include "gpio.h"

const char* TAG_SENSOR = "SENSOR";
const char* TAG_CONFIG = "CONFIG";

esp_err_t gpio_init(uint32_t intr_type, uint32_t mode, uint32_t bit_mask, uint32_t pull_down, uint32_t pull_up){
    gpio_config_t io_config = {
        .intr_type = intr_type,
        .mode = mode,
        .pin_bit_mask = bit_mask,
        .pull_down_en = pull_down,
        .pull_up_en = pull_up
    };
    return gpio_config(&io_config);
}

esp_err_t gpio_config_intr(uint32_t pin, ISR isr){
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(pin, isr, NULL);
    return ESP_OK;
}

uint32_t gpio_read_pin(uint32_t pin){
    return gpio_get_level(pin);
}

void gpio_write_pin(uint32_t pin, uint8_t value){
    gpio_set_level(pin, value);
}


