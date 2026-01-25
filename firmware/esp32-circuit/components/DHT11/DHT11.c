/**
 * @file DHT11.c
 * @brief Implementions DHT11 sensor functions
 * @author Jose Manuel Enriquez Baena (joseenriquezbaena@gmail.com)
 * @date 23-01-2026
 * @version 2.1
 */

#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "./include/DHT11.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define HIGH 1
#define LOW 0
#define BYTE 0x00000000000000FF
#define TIMEOUT 1000000

esp_err_t dht11_init(uint32_t pin){
    gpio_config_t io_config;
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pin_bit_mask = (1ULL << pin);
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    return gpio_config(&io_config);
}

esp_err_t dht11_read_humidity_integral(uint32_t pin, uint8_t* humidity){
    uint64_t data;
    int checksum = 0;
    esp_err_t err = dht11_receive_data(pin, &data);
    if(err != ESP_OK){
        return err;
    }
    *humidity = (data >> 32) & BYTE;
    checksum = ((data >> 32) & BYTE) + ((data >> 24) & BYTE) + ((data >> 16) & BYTE) + ((data >> 8) & BYTE);
    if(checksum != (data & BYTE)){
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t dht11_read_humidity_decimal(uint32_t pin, uint8_t* humidity){
    uint64_t data;
    int checksum = 0;
    esp_err_t err = dht11_receive_data(pin, &data);
    if(err != ESP_OK){
        return err;
    }
    *humidity = (data >> 24) & BYTE;
    checksum = ((data >> 32) & BYTE) + ((data >> 24) & BYTE) + ((data >> 16) & BYTE) + ((data >> 8) & BYTE);
    if(checksum != (data & BYTE)){
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t dht11_read_temperature_integral(uint32_t pin, uint8_t* temperature){
    uint64_t data;
    int checksum = 0;
    esp_err_t err = dht11_receive_data(pin, &data);
    if(err != ESP_OK){
        return err;
    }
    *temperature = (data >> 16) & BYTE;
    checksum = ((data >> 32) & BYTE) + ((data >> 24) & BYTE) + ((data >> 16) & BYTE) + ((data >> 8) & BYTE);
    if(checksum != (data & BYTE)){
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t dht11_read_temperature_decimal(uint32_t pin, uint8_t* temperature){
    uint64_t data;
    int checksum = 0;
    esp_err_t err = dht11_receive_data(pin, &data);
    if(err != ESP_OK){
        return err;
    }
    *temperature = (data >> 8) & BYTE;
    checksum = ((data >> 32) & BYTE) + ((data >> 24) & BYTE) + ((data >> 16) & BYTE) + ((data >> 8) & BYTE);
    if(checksum != (data & BYTE)){
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t dht11_read(uint32_t pin, uint8_t* humidity_int, uint8_t* humidity_dec, uint8_t* temperature_int, uint8_t* temperature_dec){
    uint64_t data;
    uint8_t checksum_calculated = 0;
    uint8_t checksum_received;
    esp_err_t err = dht11_receive_data(pin, &data);

    if(err != ESP_OK) return err;
    
    *humidity_int = (data >> 32) & BYTE;
    checksum_calculated += *humidity_int;
    
    *humidity_dec = (data >> 24) & BYTE;
    checksum_calculated += *humidity_dec;
    
    *temperature_int = (data >> 16) & BYTE;
    checksum_calculated += *temperature_int;
    
    *temperature_dec = (data >> 8) & BYTE;
    checksum_calculated += *temperature_dec;
    
    checksum_received = data & BYTE;

    if(checksum_calculated != checksum_received){
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}

esp_err_t dht11_wakeup_sensor(uint32_t pin){
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, LOW);
    int64_t start_time = esp_timer_get_time();
    while((esp_timer_get_time() - start_time) == 18000); // wait 18ms
    gpio_set_level(pin, HIGH);
    return ESP_OK;
}

esp_err_t dht11_await_response_sensor(uint32_t pin){
    
    if(gpio_set_direction(pin, GPIO_MODE_INPUT) != ESP_OK){
        return ESP_ERR_INVALID_ARG;
    }
    
    int64_t start_time = esp_timer_get_time();
    while(gpio_get_level(pin) == HIGH){ // DHT11 responds by sending a low level to the bus
        if(esp_timer_get_time() - start_time > TIMEOUT){ // if the mcu doesnt receive a signal in 1 second, sends a error.
            return ESP_ERR_TIMEOUT;
        }
    } 

    start_time = esp_timer_get_time();
    while(gpio_get_level(pin) == LOW){ // DHT11 is ready to send data
        if(esp_timer_get_time() - start_time > TIMEOUT){
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

esp_err_t dht11_receive_bits(uint32_t pin, int64_t* intervals){
    for(int i = 0; i < 40; i++){
        while(gpio_get_level(pin) == HIGH);
        while(gpio_get_level(pin) == LOW);
        int start_time = esp_timer_get_time();
        while(gpio_get_level(pin) == HIGH); 
        intervals[i] = esp_timer_get_time() - start_time;
    }
    return ESP_OK;
}

esp_err_t dht11_receive_data(uint32_t pin, uint64_t* data){
    int64_t intervals[40];

    esp_err_t err = dht11_wakeup_sensor(pin);
    if(err != ESP_OK){
        return err;
    }

    err = dht11_await_response_sensor(pin);
    if(err != ESP_OK){
        return err;
    }   

    static portMUX_TYPE myMux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&myMux);
    err = dht11_receive_bits(pin, intervals);
    portEXIT_CRITICAL(&myMux);

    *data = 0;
    for(int i = 0; i < 40; i++){
        if(intervals[i] > 50){
            *data = (*data << 1) | 1;
        }else{
            *data = (*data << 1) | 0;
        }
    }
    return ESP_OK;
}

