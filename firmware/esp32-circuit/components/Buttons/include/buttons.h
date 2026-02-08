#ifndef BUTTONS_H
#define BUTTONS_H

#include "gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "board_definition.h"

typedef enum{
    BUTTON_OK,
    BUTTON_ERR_INVALID
}Button_err_t;

typedef void(*button_callback)(uint32_t io_num);

Button_err_t buttons_init(button_callback callback);
void vButtonISRTask(void* arg);


#endif