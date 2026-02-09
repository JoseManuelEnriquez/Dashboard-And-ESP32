#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

/**
 * @file communications.h
 * @brief Modulo de manejo de MQTT y gestion de comunicaciones
 * @author Jose Manuel Enriquez Baena
 * @date 08-02-2026
 */

#include "mqtt_client.h"
#include "esp_log.h"
#include "frozen.h" // Libreria necesaria para crear json strings
#include "board_definition.h"

#define MAX_LEN_DEVICE 10
#define MAX_LEN_TOPIC 64

/**
 * @brief Estructura que agrupa los datos enviados al topico telemetria
 */
typedef struct{
    uint8_t humicity;
    uint8_t temperature;
    uint8_t light;
}data_t;

/**
 * @brief Especifica los errores que ocurren en MQTT
 */
typedef enum{
    MQTT_OK,
    MQTT_ERR_INVALID
}eMQTT_err;

/**
 *  @brief Topicos a los que el modulo esta suscrito
 */
typedef enum{
    MQTT_ON,
    MQTT_SLEEP,
    MQTT_CONFIG,
    MQTT_DELAY
}eMQTT_topic;

/**
 *  @brief Estructura que define que tipo de mensaje que recibe por parametros la callback
 */
typedef struct{
    eMQTT_err status;
    eMQTT_topic topic;
    char* data;
}mqtt_message_t;

typedef void(*mqtt_callback)(mqtt_message_t message);
extern const char* TAG_MQTT;

// Las variables se definen en un archivo privado por seguridad.
extern const char* broker_uri;
extern const char* username;
extern const char* password;

/**
 * @brief Configuracion y conexion con el broker MQTT
 * @param callback Funcion para recibir los datos de la suscripcion a los topicos
 * @details Configura los parametros necesarios como el broker uri, credenciales, client_id para poder
 * iniciar cliente MQTT y registrar el manejor de eventos MQTT
 */
void mqtt_app_start(mqtt_callback callback, char device[MAX_LEN_DEVICE], int id);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void publish_data(data_t* data);
#endif