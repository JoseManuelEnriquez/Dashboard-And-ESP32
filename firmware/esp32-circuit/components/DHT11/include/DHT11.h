/**
 * @file DHT11.h
 * @brief Definitions DHT11 sensor functions
 * @author Jose Manuel Enriquez Baena (joseenriquezbaena@gmail.com)
 * @date 23-01-2026
 * @version 2.1   
 */

#ifndef DHT11_H
#define DHT11_H
#include "driver/gpio.h"

/** 
 * ----------------------------------------
 *  High-Level DHT11 sensor functions
 * ----------------------------------------
 */

 /**
 * @brief Initialize the GPIO number which is connected the DHT11 sensor
 * @param pin The GPIO number
 * @return ESP_OK: The configuration is successful, ESP_ERR_INVALID_ARG: arguments are invalid
 */
esp_err_t dht11_init(uint32_t pin);

/**
 * @brief Read the data and return the humidity and the temperature
 * @param pin The GPIO number
 * @param humidity_int [out] Pointer to store the humidity integral
 * @param humidity_dec [out] Pointer to store the humidity decimal
 * @param temperature_int [out] Pointer to store the temperature integral
 * @param temperature_dec [out] Pointer to store the temperature decimal
 * @return ESP_OK ESP_ERR_INVALID_CRC ESP_ERR_INVALID_ARG
 */
esp_err_t dht11_read(uint32_t pin, uint8_t* humidity_int, uint8_t* humidity_dec, uint8_t* temperature_int, uint8_t* temperature_dec);

/**
 * @brief Read the data and return the humidity integral
 * @param pin The GPIO number
 * @param humidity_int [out] Pointer to store the humidity integral
 * @return ESP_OK ESP_ERR_INVALID_CRC ESP_ERR_INVALID_ARG
 */
esp_err_t dht11_read_humidity_integral(uint32_t pin, uint8_t* humidity);

/**
 * @brief Read the data and return the humidity decimal
 * @param pin The GPIO number
 * @param humidity_int [out] Pointer to store the humidity decimal
 * @return ESP_OK ESP_ERR_INVALID_CRC ESP_ERR_INVALID_ARG
 */
esp_err_t dht11_read_humidity_decimal(uint32_t pin, uint8_t* humidity);

/**
 * @brief Read the data and return the temperature integral
 * @param pin The GPIO number
 * @param humidity_int [out] Pointer to store the temperature integral
 * @return ESP_OK ESP_ERR_INVALID_CRC ESP_ERR_INVALID_ARG
 */
esp_err_t dht11_read_temperature_integral(uint32_t pin, uint8_t* temperature);

/**
 * @brief Read the data and return the temperature decimal
 * @param pin The GPIO number
 * @param humidity_int [out] Pointer to store the temperature decimal
 * @return ESP_OK ESP_ERR_INVALID_CRC ESP_ERR_INVALID_ARG
 */
esp_err_t dht11_read_temperature_decimal(uint32_t pin, uint8_t* temperature);

/** 
 * ------------------------------------------
 *  Communications control functions
 * ------------------------------------------
 */

 /**
 * @brief The MCU wakes up the DHT11 sensor 
 * @param pin The GPIO number 
 * @return ESP_OK: DHT11 has succesfully woken up, ESP_ERR_INVALID_ARG: arguments are invalid
 */
esp_err_t dht11_wakeup_sensor(uint32_t pin);

/**
 * @brief The MCU wait for a response from DHT11 sensor
 * @param pin The GPIO number
 * @return ESP_OK: DHT11 has responded, ESP_ERR_INVALID_ARG: arguments are invalid ESP_ERR_TIMEOUT 
 */
esp_err_t dht11_await_response_sensor(uint32_t pin);

/**
 * @brief Control to receive a bit
 * @param pin The GPIO number
 * @param intervals [out] Vector with 40 intervals
 * @return ESP_OK: Reading is succesfull 
 */
esp_err_t dht11_receive_bits(uint32_t pin, int64_t* intervals);

/**
 * @brief Function to receive all the data (40 bits)
 * @param pin The GPIO number
 * @param data [out] Pointer to store the data
 * @return ESP_OK, ESP_ERR_INVALID_ARG
 */
esp_err_t dht11_receive_data(uint32_t pin, uint64_t* data);

#endif