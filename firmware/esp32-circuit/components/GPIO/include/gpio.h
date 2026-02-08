#ifndef GPIO_H
#define GPIO_H

#include "esp_log.h"
#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define LOW 0
#define HIGH 1

typedef void(*ISR)(void* args);

esp_err_t gpio_init(uint32_t intr_type, uint32_t mode, uint32_t bit_mask, uint32_t pull_down, uint32_t pull_up);
uint32_t gpio_read_pin(uint32_t pin);
void gpio_write_pin(uint32_t pin, uint8_t value);
esp_err_t gpio_config_intr(uint32_t pin, ISR isr);

#endif