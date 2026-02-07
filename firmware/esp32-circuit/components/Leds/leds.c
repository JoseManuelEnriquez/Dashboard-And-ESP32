#include "leds.h"

led_err_t led_init(){
    esp_err_t err;
    uint32_t bitmask = (1ULL << LED_RED | 1ULL << LED_GREEN | 1ULL<< LED_YELLOW | 1ULL << CONNECTED_LED | 1ULL << CONFIGURATION_LED);
    err = gpio_init(GPIO_INTR_DISABLE, GPIO_MODE_OUTPUT, bitmask, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE);
    return (err == ESP_OK) ? LED_OK : LED_ERR_INVALID;
}

void led_on(uint32_t pin){
    gpio_set_level(pin, HIGH);
}

void led_off(uint32_t pin){
    gpio_set_level(pin, LOW);
}

/*
Esta funcion es necesaria para los leds que se encienden para indicar si hay conexion o no. Se manda por parametros
al realizar el init de wifi.
*/
void callback_wifi(int connect){
    if(connect == 1){
        gpio_set_level(CONNECTED_LED, HIGH);
        gpio_set_level(CONFIGURATION_LED, LOW);
    }else{
        gpio_set_level(CONFIGURATION_LED, HIGH);
        gpio_set_level(CONNECTED_LED, LOW);
    }
}