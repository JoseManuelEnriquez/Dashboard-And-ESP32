#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

/**
 * @file communications.h
 * @brief Modulo gestion de comunicaciones
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
}comm_telemetry_t;

/**
 * @brief Especifica los errores que ocurren en MQTT
 */
typedef enum{
    COMM_OK,
    COMM_ERR_INVALID
}eComm_err;

/**
 *  @brief Topicos a los que el modulo esta suscrito
 */
typedef enum{
    ON,
    SLEEP,
    CONFIG,
    DELAY
}eComm_message_type;

/**
 *  @brief Estructura que define que tipo de mensaje que recibe por parametros la callback
 */
typedef struct{
    eComm_err status;
    eComm_message_type message_type;
    char* data;
}comm_message_t;

typedef void(*comm_callback)(comm_message_t message);

/**
 * @brief Configuracion y conexion con el broker MQTT
 * @param callback Funcion para recibir los datos de la suscripcion a los topicos
 * @details Configura los parametros necesarios como el broker uri, credenciales, client_id para poder
 * iniciar cliente MQTT y registrar el manejor de eventos MQTT
 */
void comm_init(comm_callback callback, char* device, int id);
eComm_err comm_send_telemetry(comm_telemetry_t* data);
#endif