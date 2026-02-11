#ifndef BUTTONS_H
#define BUTTONS_H

/**
 * @file buttons.h
 * @brief Módulo para inicializacion, configuracion y uso de botones.
 * @author Jose Manuel Enriquez Baena
 * @date 08-02-2026
 */

#include "gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "board_definition.h"

/**
 * Enum para poder definir los diferentes tipos de errores que puede devolver las funciones.
 */
typedef enum{
    BUTTON_OK,
    BUTTON_ERR_INVALID
}Button_err_t;

typedef void(*button_callback)(uint32_t io_num);

/**
 * @brief Inicializa los botones
 * @param callback Funcion callback que se llama cuando ocurre una interrupcion
 * @details Inicializa los pines GPIO correspondientes definidos en board_definition.h y las interrupciones
 *          que utiliza el sistema. 
 * 
 *          Para las interrupciones se usa el concepto de Deferred Interrupt Processing.
 *          Se crea una tarea vButtonISRTask que estara bloqueada hasta que alguna ISR la despierte. En
 *          esta tarea ejecuta la callback que se pasa por parametros.
 */
Button_err_t buttons_init(button_callback callback);

/**
 * @brief Tarea que llama a la callback
 */
void vButtonISRTask(void* arg);


#endif