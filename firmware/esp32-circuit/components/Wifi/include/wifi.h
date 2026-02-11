#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"


#define CONFIG_ESP_MAXIMUM_RETRY 3
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

typedef void(*WifiCallback_t)(int conectado);

void wifi_init_sta(WifiCallback_t callback);
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif