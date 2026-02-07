#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H
#include "mqtt_client.h"
#include "esp_log.h"
#include "frozen.h" // Libreria necesaria para crear json strings

/**
 * @brief Estructura de datos que almacena los datos leidos por los sensores
 */
typedef struct{
    uint8_t humicity;
    uint8_t temperature;
    uint8_t light;
}data_t;

// static int delay;               // Tiempo que se queda bloqueada la tarea ReadSensor (es configurable por MQTT)
extern const char* TAG_MQTT;

// Las variables se definen en un archivo privado por seguridad.
extern const char* broker_uri;
extern const char* username;
extern const char* password;

void mqtt_app_start();
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void publish_data(data_t* data);
#endif