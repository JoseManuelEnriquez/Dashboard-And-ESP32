/**
 * @file communications.c
 * @brief Implementacion de logica de comunicacion y publicacion MQTT.
 */

#include "communications.h"

typedef struct{
    char on_topic [MAX_LEN_TOPIC];
    char sleep_topic [MAX_LEN_TOPIC];
    char config_topic [MAX_LEN_TOPIC];
    char delay_topic [MAX_LEN_TOPIC];
    char telemetry_topic [MAX_LEN_TOPIC];
}mqtt_topics_t;

static mqtt_topics_t gTopics;
static esp_mqtt_client_handle_t client; // client debe ser global para poder publicar desde publish_data()

static int id_device;
static comm_callback callback_private;

const static char* TAG_MQTT = "MQTT";
const static char* broker_uri = CONFIG_BROKER_URI;
const static char* username = CONFIG_USERNAME;
const static char* password = CONFIG_PASSWORD;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // esp_mqtt_event_handle_t es una macro que es un puntero a esp_mqtt_event_t (estructura con los diferentes campos)
    esp_mqtt_event_handle_t event = event_data;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_BIDEFORE_CONNECT");
        break;
    case MQTT_EVENT_CONNECTED: 
        msg_id = esp_mqtt_client_subscribe(client, gTopics.on_topic, 0);
        msg_id = esp_mqtt_client_subscribe(client, gTopics.sleep_topic, 0);
        msg_id = esp_mqtt_client_subscribe(client, gTopics.config_topic, 0);
        msg_id = esp_mqtt_client_subscribe(client, gTopics.delay_topic, 0);
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
        comm_message_t message;
        if(strcmp(topic, gTopics.on_topic) == 0)
        {
            message.message_type = ON;
            message.status = COMM_OK;
            callback_private(message);
        }else if(strcmp(topic, gTopics.config_topic) == 0)
        {
            message.message_type = CONFIG;
            message.status = COMM_OK;
            callback_private(message);
        }else if(strcmp(topic, gTopics.sleep_topic) == 0)
        {
            message.message_type = SLEEP;
            message.status = COMM_OK;
            callback_private(message);
        }else if(strcmp(topic, gTopics.delay_topic) == 0)
        {
            message.message_type = DELAY;
            message.status = COMM_OK;
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

void comm_init(comm_callback callback, char* device, int id)
{
    callback_private = callback;
    id_device = id;

    sprintf(gTopics.on_topic, MAX_LEN_TOPIC, "%s/%d/config/ON", device, id);
    sprintf(gTopics.sleep_topic, MAX_LEN_TOPIC, "%s/%d/config/SLEEP", device, id);
    sprintf(gTopics.config_topic, MAX_LEN_TOPIC, "%s/%d/config/CONFIG", device, id);
    sprintf(gTopics.delay_topic, MAX_LEN_TOPIC, "%s/%d/config/DELAY", device, id);
    sprintf(gTopics.telemetry_topic, MAX_LEN_TOPIC, "%s/%d/telemetry", device, id);
    
    /**
        No se configura id_cliente porque usa por defecto: ESP32_CHIPID% donde CHIPID% son los
        ultimos 3 bytes(hex) de la MAC.
    */
    esp_mqtt_client_config_t mqtt_conf = {
        .broker.address.uri = broker_uri,
        .credentials.username = username,
        .credentials.authentication.password = password
    };
    client = esp_mqtt_client_init(&mqtt_conf);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    ESP_LOGI(TAG_MQTT,"APP MQTT START\n");
}

eComm_err comm_send_telemetry(comm_telemetry_t* data){
    char buffer[128];

    char temperature_topic[MAX_LEN_TOPIC];
    char humidicity_topic[MAX_LEN_TOPIC];
    char light_topic[MAX_LEN_TOPIC];

    /**
     * Se debe generar un topico por cada magnitud fisica segun viene especificado en la jerarquia topica
     */
    sprintf(temperature_topic, MAX_LEN_TOPIC, "%s/temperature", gTopics.telemetry_topic);
    sprintf(humidicity_topic, MAX_LEN_TOPIC, "%s/humidicity", gTopics.telemetry_topic);
    sprintf(light_topic, MAX_LEN_TOPIC, "%s/light", gTopics.telemetry_topic);

    struct json_out out_temperature = JSON_OUT_BUF(buffer, sizeof(buffer));
    json_printf(&out_temperature, "{id: %d, temperature: %d, unidad: %s}", id_device, data->temperature, "Celsius");    
    esp_mqtt_client_publish(client, temperature_topic, buffer, 0, 0, 0);
    
    struct json_out out_humidicity = JSON_OUT_BUF(buffer, sizeof(buffer));
    json_printf(&out_humidicity, "{id: %d, humidicity: %d, unidad: %s}", id_device, data->humicity, "percentage");
    esp_mqtt_client_publish(client, humidicity_topic, buffer, 0, 0, 0);

    struct json_out out_light = JSON_OUT_BUF(buffer, sizeof(buffer));
    json_printf(&out_light, "{id: %d, light: %d, unidad: %s}", id_device, data->light, "bool");
    esp_mqtt_client_publish(client, light_topic, buffer, 0, 0, 0);

    return COMM_OK;
}
