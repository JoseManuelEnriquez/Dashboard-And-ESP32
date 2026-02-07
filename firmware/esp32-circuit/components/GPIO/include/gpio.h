#ifndef GPIO_H
#define GPIO_H

#include "esp_log.h"
#include "esp_driver_gpio"

#define LOW 0
#define HIGH 1

// Etiquetas para la funcion ESP_LOG
extern const char* TAG_SENSOR;
extern const char* TAG_CONFIG;

void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green);
esp_err_t button_config();
esp_err_t leds_config();
esp_err_t ldr_config();

#endif