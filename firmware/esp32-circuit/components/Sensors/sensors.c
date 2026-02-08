#include "sensors.h"

/** 
 * Variable necesaria para controlar si esta en estado on/off. 
 * Si state = 1 --> on
 * Si state = 0 --> off
*/
static int state = 0;

eSensor_error sensors_init(){
    esp_err_t err = gpio_init(GPIO_INTR_DISABLE, GPIO_MODE_INPUT, (1ULL << LDR_SENSOR), GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_DISABLE);
    if(err != ESP_OK) return SENSOR_ERR_INVALID;

    err = dht11_init(DHT11_SENSOR);
    if(err != ESP_OK) return SENSOR_ERR_INVALID;

    return SENSOR_OK;
}

void sensors_on(){
    state = 1;
}
void sensors_off(){
    state = 0;
}

eSensor_error readSensors(sensor_data_t* data){
    if(state == 1){
        uint8_t light = gpio_read_pin(LDR_SENSOR);
        uint8_t temperature_int, temperature_dec, humidicity_int, humidicity_dec;
        esp_err_t err = dht11_read(DHT11_SENSOR, &humidicity_int, &humidicity_dec, &temperature_int, &temperature_dec);
        if(err != ESP_OK) return SENSOR_ERR_READ;

        data->light = light;
        data->humidicity = humidicity_int;
        data->temperature = temperature_int;

        return SENSOR_OK;
    }else{
        return SENSOR_ERR_STATE;
    }
}