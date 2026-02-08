/**
 * @file communications.c
 * @brief Implementacion de logica de comunicacion y publicacion MQTT.
 */

#include "communications.h"

static mqtt_callback callback_private;
static esp_mqtt_client_handle_t client; // client debe ser global para poder publicar desde publish_data()
const static char* TAG_MQTT = "MQTT";
const static char* broker_uri = CONFIG_BROKER_URI;
const static char* username = CONFIG_USERNAME;
const static char* password = CONFIG_PASSWORD;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // esp_mqtt_event_handle_t es una macro que es un puntero a esp_mqtt_event_t (estructura con los diferentes campos)
    esp_mqtt_event_handle_t event = event_data;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_BEFORE_CONNECT");
        break;
    case MQTT_EVENT_CONNECTED: 
        msg_id = esp_mqtt_client_subscribe(client, "ESP32/1/config/ON", 0);
        msg_id = esp_mqtt_client_subscribe(client, "ESP32/1/config/SLEEP", 0);
        msg_id = esp_mqtt_client_subscribe(client, "ESP32/1/config/CONFIG", 0);
        msg_id = esp_mqtt_client_subscribe(client, "ESP32/1/config/delay", 0);
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
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

        char topic[40];
        sprintf(topic, "%.*s", event->topic_len,event->topic);

        /* 
            ESP32 esta suscrito a varios topicos:
            - ESP32/1/config/ON: Cambia el modo del ESP32 a modo performance
            - ESP32/1/config/configuration: Cambia el modo del ESP32 a modo configuration
            - ESP32/1/config/SLEEP: Cambia el modo del ESP32 a modo off
            - ESP32/1/config/delay: Topico para cambiar el delay que espera la tarea ReadSensor. 
                Para cambiar el delay ESP32 debe de estar en modo configuracion. En caso que se reciba
                un mensaje config/delay y no esta en modo configuration, da el error "INCORRECT MODE"
                Este topico tiene de payload:
                {
                    delay: value
                }
                Si value < MIN_DELAY, salta un error "INCORRECT DELAY"
        */
        ESP_LOGI(TAG_MQTT, "TOPIC: %.*s", event->topic_len, event->topic);
        mqtt_message_t message;
        if(strcmp(topic, "ESP32/1/config/ON") == 0)
        {
            message.topic = MQTT_ON;
            message.status = MQTT_OK;
            callback_private(message);
        }else if(strcmp(topic, "ESP32/1/config/CONFIG") == 0)
        {
            message.topic = MQTT_CONFIG;
            message.status = MQTT_OK;
            callback_private(message);
        }else if(strcmp(topic, "ESP32/1/config/SLEEP") == 0)
        {
            message.topic = MQTT_SLEEP;
            message.status = MQTT_OK;
            callback_private(message);
        }else if(strcmp(topic, "ESP32/1/config/delay") == 0)
        {
            message.topic = MQTT_DELAY;
            message.status = MQTT_OK;
            message.data = event->data;
            callback_private(message);
          
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG_MQTT, "UNKNOWN EVENT id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(mqtt_callback callback)
{
    callback_private = callback;
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
        .credentials.authentication.password = password
    };
    client = esp_mqtt_client_init(&mqtt_conf);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    ESP_LOGI(TAG_MQTT,"APP MQTT START\n");
}

void publish_data(data_t* data)
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