#include "leds.h"

led_err_t led_init(){
    esp_err_t err;
    uint32_t bitmask = (1ULL << IDLE_LED | 1ULL << PERFORMANCE_LED | 1ULL<< CONFIG_LED | 1ULL << CONNECTED_LED | 1ULL << CONFIGURATION_LED);
    err = gpio_init(GPIO_INTR_DISABLE, GPIO_MODE_OUTPUT, bitmask, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE);
    return (err == ESP_OK) ? LED_OK : LED_ERR_INVALID;
}

void led_on(uint32_t pin){
    gpio_set_level(pin, HIGH);
}

void led_off(uint32_t pin){
    gpio_set_level(pin, LOW);
}