/**
 * @file main.c
 * @author Jose Manuel Enriquez Baena (joseenriquezbaena@gmail.com)
 * @brief Lectura de sensores y publicacion por MQTT
 * @version 1.4
 * @date 20-01-2026
 * @copyright Copyright (c) 2026
 * */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "frozen.h" // Libreria necesaria para crear json strings
#include "DHT11.h"
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * -----------------------------------------------------------
 * DEFINICIONES MACROS | VARIABLES GLOBALES | TIPOS DE DATOS
 * -----------------------------------------------------------
 */

#define ID 1  

#define LED_RED GPIO_NUM_23
#define LED_GREEN GPIO_NUM_21
#define LED_YELLOW GPIO_NUM_22
#define CHANGE_BUTTON GPIO_NUM_26
#define OFF_BUTTON GPIO_NUM_27
#define DHT11_SENSOR GPIO_NUM_14
#define LDR_SENSOR GPIO_NUM_19

#define ESP_INTR_FLAG_DEFAULT 0

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define CONFIG_ESP_MAXIMUM_RETRY 3

#define LOW 0
#define HIGH 1

/**
 *  @brief Estados principales del dispositivo
 *  
 *  - Performance: Lectura periodica de los sensores y envio al broker MQTT
 *  - configuration:
 *  - off: No se realiza lecturas del sensor el dispositivo (modo sleep)
 */
typedef enum
{
    performance,
    configuration,
    off
} State_t;

/**
 * @brief Estructura de datos que almacena los datos leidos por los sensores
 */
typedef struct{
    uint8_t humicity;
    uint8_t temperature;
    uint8_t light;
}data_t;

static QueueHandle_t isr_handler_queue = NULL; // Para despertar la tarea ChangeState
static State_t currentState = off; 
esp_mqtt_client_handle_t client; // client debe ser global para poder publicar desde publish_data()
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static int mqtt_connected = 0;

// Etiquetas para la funcion ESP_LOG
static const char* TAG_SENSOR = "SENSOR_TASK";
static const char* TAG_CONFIG = "CONFIG";
static const char* TAG_MQTT = "MQTT";
static const char* TAG_WIFI = "WIFI";

// Las variables se definen en un archivo privado por seguridad.
static const char* broker_uri = CONFIG_MQTT_BROKER_URI;
static const char* username = CONFIG_USERNAME;
static const char* password = CONFIG_PASSWORD;

/**
 * -------------------------------------------------
 * PROTOTIPO DE FUNCIONES
 * -------------------------------------------------
 */

static void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green);
static esp_err_t button_config();
static esp_err_t leds_config();
static esp_err_t ldr_config();
static void publish_data(data_t* data);
static void mqtt_app_start();
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void wifi_init_sta();
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * -------------------------------------------------
 * FUNCIONES DE TAREAS freeRTOS
 * -------------------------------------------------
 */

void vMonitorTask(void *pvParameters) {
    char buffer[400];
    for (;;) {
        printf("\n--- Task List ---\n");
        printf("Nombre          Estado  Prio  Stack   ID\n");
        vTaskList(buffer);
        printf("%s", buffer);
        printf("-----------------\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void vControlFSMTask(void* pvParameters)
{
    for(;;){
        switch (currentState)
        {
        case performance:
            set_io_level(LOW, LOW, HIGH); // RED = OFF, YELLOW = OFF, GREEN = HIGH
            break;
        case configuration:
            set_io_level(LOW, HIGH, LOW); // RED = OFF, YELLOW = HIGH, GREEN = LOW
            break;
        case off:
            set_io_level(HIGH, LOW, LOW); // RED = HIGH, YELLOW = OFF, GREEN = LOW
            break;
        default:
            currentState = off;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

static void vChangeStateTask(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(isr_handler_queue, &io_num, portMAX_DELAY)) {
            if(io_num == OFF_BUTTON){
                currentState = off;
            }else{
                if(currentState == performance){
                    currentState = configuration;
                }else if(currentState == configuration){
                    currentState = performance;
                }else{
                    currentState = performance;
                }
            }
        }
    }
    vTaskDelete(NULL);
}

void vReadSensorTask(void* pvParameters)
{
    data_t data;
    // uint8_t humicity_int, humicity_dec, temperature_int, temperature_dec;
    esp_err_t err;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;){
        switch(currentState){
            case performance:
            // err = dht11_read(DHT11_SENSOR, &humicity_int, &humicity_dec, &temperature_int, &temperature_dec);
            err = ESP_OK;
            if(err == ESP_OK){
                data.light = gpio_get_level(LDR_SENSOR);
                // data.humicity = humicity_int;
                // data.temperature = temperature_int;
                if(mqtt_connected == 1)
                    publish_data(&data);                
            }else{
                switch (err)
                {
                case ESP_ERR_INVALID_ARG: ESP_LOGE(TAG_SENSOR, "ESP_ERR_INVALID_ARG");
                break;
                case ESP_ERR_INVALID_CRC: ESP_LOGE(TAG_SENSOR, "ESP_ERR_INVALID_CRC");
                break;
                case ESP_ERR_TIMEOUT: ESP_LOGE(TAG_SENSOR, "ESP_ERR_TIMEOUT");
                break;
                default:
                    break;
                }
            }
            break;
            case configuration:
            break;
            case off:
            break;
            default:
            break;
        }
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL);
}

/**
 * -------------------------------------------------
 * FUNCIONES INTERRUPCIONES
 * -------------------------------------------------
 */
static void IRAM_ATTR gpio_isr_change_button_handler(void* args) // Manejador de interrupcion
{   
    uint32_t io_num = CHANGE_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL);
}

static void IRAM_ATTR gpio_isr_off_button_handler(void* args){
    uint32_t io_num = OFF_BUTTON;
    xQueueSendFromISR(isr_handler_queue, &io_num, NULL); 
}   

/**
 * -------------------------------------------------
 * MAIN
 * -------------------------------------------------
 */
void app_main(void)
{
    // ------ CONFIGURATION ------
    ESP_ERROR_CHECK(leds_config());
    ESP_ERROR_CHECK(ldr_config());
    ESP_ERROR_CHECK(button_config());
    ESP_ERROR_CHECK(dht11_init(DHT11_SENSOR));
    ESP_LOGI(TAG_CONFIG, "HARDWARE INIT SUCCESS\n");
    isr_handler_queue = xQueueCreate(10, sizeof(uint32_t));
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    ESP_LOGI(TAG_WIFI, "WIFI INIT SUCCESS");
    
    mqtt_app_start();
    // ------ CREATION TASKS ------

    // Gestiona los estados de la FSM
    xTaskCreate(vControlFSMTask,"FSM Control Task", 2048, NULL, 6, NULL); 
    
    // Lee los sensores en modo performance.
    xTaskCreate(vReadSensorTask,"Read Sensor Task", 4096, NULL, 6, NULL); 
    
    // Despertada por la ISR para cambiar de estado. Prioridad maxima para respuesta inmediata
    xTaskCreate(vChangeStateTask,"Change State Task", 4096, NULL, 7, NULL);
    
    // Tarea necesaria para hacer debug de los estados y caracteristicas de todas las tareas del sistema
    // xTaskCreate(vMonitorTask,"Monitor Task",4096, NULL, , NULL);
}

/**
 * -------------------------------------------------
 * DEFINICION DE FUNCIONES
 * -------------------------------------------------
 */

static esp_err_t ldr_config(){
    gpio_config_t ldr_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << LDR_SENSOR),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    return gpio_config(&ldr_config);
}

static esp_err_t leds_config()
{
    // Configuracion de pines
    gpio_config_t out_pin = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << LED_RED | 1ULL << LED_GREEN | 1ULL << LED_YELLOW),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    return gpio_config(&out_pin);
}

static esp_err_t button_config()
{
    // Configuracion de pines
    esp_err_t err;
    gpio_config_t input_pin = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << CHANGE_BUTTON | 1ULL << OFF_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    err = gpio_config(&input_pin);

    // Configuracion de interrupciones
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(CHANGE_BUTTON, gpio_isr_change_button_handler, NULL);
    gpio_isr_handler_add(OFF_BUTTON, gpio_isr_off_button_handler, NULL);

    return err;
}

static void set_io_level(uint32_t level_red, uint32_t level_yellow, uint32_t level_green)
{
    gpio_set_level(LED_RED, level_red);
    gpio_set_level(LED_YELLOW, level_yellow);
    gpio_set_level(LED_GREEN, level_green);
}

static void publish_data(data_t* data)
{
    /*
        Hay que crear una instancia de json_out para cada magnitud porque sino da fallo el compilador
        Por como se define la jerarquita topica es:
        <dispositivo>/<id>/telemetry/<magnitud-fisica>
        
        El payload:
        {
            id
            valor
            unidad
        }
    */
    char buffer[128];
    struct json_out out_temperature = JSON_OUT_BUF(buffer, sizeof(buffer));
    json_printf(&out_temperature, "{id: %d, temperature: %d, unidad: %s}", ID, data->temperature, "Celsius");
    esp_mqtt_client_publish(client, "ESP32/1/telemetry/temperature", buffer, 0, 0, 0);
    
    struct json_out out_humidicity = JSON_OUT_BUF(buffer, sizeof(buffer));
    json_printf(&out_humidicity, "{id: %d, humidicity: %d, unidad: %s}", ID, data->humicity, "percentage");
    esp_mqtt_client_publish(client, "ESP32/1/telemetry/humidicity", buffer, 0, 0, 0);

    struct json_out out_light = JSON_OUT_BUF(buffer, sizeof(buffer));
    json_printf(&out_light, "{id: %d, light: %d, unidad: %s}", ID, data->light, "bool");
    esp_mqtt_client_publish(client, "ESP32/1/telemetry/light", buffer, 0, 0, 0);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // esp_mqtt_event_handle_t es una macro que es un puntero a esp_mqtt_event_t (estructura con los diferentes campos)
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG_MQTT, "READY EVENT");
        break;
    case MQTT_EVENT_CONNECTED: 
        mqtt_connected = 1;
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = 0;
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;
     case MQTT_EVENT_PUBLISHED:
        ESP_LOGE(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGE(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\n", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG_MQTT, "UNKNOWN EVENT id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start()
{
    /*
        No se configura id_cliente porque usa por defecto: ESP32_CHIPID% donde CHIPID% son los
        ultimos 3 bytes(hex) de la MAC.
        
        !! OJO: Por ahora dejar configuracion por defecto los campos network_t, buffer_t, session_t, topic_t
        session_t y topic_t es para Lass_will o ultima voluntad
    */
    esp_mqtt_client_config_t mqtt_conf = {
        .broker.address.uri = broker_uri,
        .credentials.username = username,
        .credentials.client_id = "ESP32",
        //.network.timeout_ms = 10000,
        .credentials.authentication.password = password
    };
    client = esp_mqtt_client_init(&mqtt_conf);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    ESP_LOGI(TAG_MQTT,"APP MQTT START\n");
}

static void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "WIFI_INIT_STA FINISHED");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_WIFI, "CONNECTED");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG_WIFI, "CONNECTED FAILED");
    } else {
        ESP_LOGE(TAG_WIFI, "NO EXPECTED EVENT");
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "RETRY CONNECTION");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGE(TAG_WIFI,"CONECTION AP FAILED");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}