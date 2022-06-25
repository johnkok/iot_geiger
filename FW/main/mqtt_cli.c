#include "mqtt_cli.h"

extern rx_store_s rx_store;

static const char *TAG = "MQTT";
static const char *CONFIG_BROKER_URL = "mqtt://192.168.17.111:1883";
uint16_t mqtt_interval = 60;
uint8_t mqtt_protocol = 0;
char mqtt_port[16];
char mqtt_username[16];
char mqtt_password[16];
char mqtt_broker[16];
esp_mqtt_client_handle_t client = NULL;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void mqtt_publish_data(esp_mqtt_client_handle_t client)
{
int msg_id;
char buffer[256];


    snprintf(buffer, sizeof(buffer),   "{\"counter\":\"%f\","
					"\"temperature\":\"%3.2f\","
					"\"humidity\":\"%3.2f\","
					"\"pm1\":\"%d\","
					"\"pm2_5\":\"%d\","
					"\"pm10\":\"%d\"}",
		    rx_store.current.gc_usv,
		    rx_store.current.temperature,
		    rx_store.current.humidity,
		    rx_store.current.pm1,
		    rx_store.current.pm2_5,
		    rx_store.current.pm10);
    msg_id = esp_mqtt_client_publish(client, "home/geiger", buffer, 0, 1, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
	
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_publish_data(client);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_init(void)
{
    nvs_handle_t nvs_handle;
    size_t required_size;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };

    if (nvs_open_from_partition("iot_geiger", "default", NVS_READONLY, &nvs_handle) == ESP_OK)
    {
        required_size = 16;
        if (nvs_get_str(nvs_handle, "mqtt_username", &mqtt_username[0], &required_size) != ESP_OK)
        {
            ESP_LOGE(TAG, "mqtt_username not in NVS");
        }
        required_size = 16;
        if (nvs_get_str(nvs_handle, "mqtt_password", &mqtt_password[0], &required_size) != ESP_OK)
        {
            ESP_LOGE(TAG, "mqtt_password not in NVS");
        }
        if (nvs_get_u16(nvs_handle, "mqtt_interval", &mqtt_interval) != ESP_OK)
        {
            ESP_LOGE(TAG, "mqtt_interval not in NVS");
        }
        required_size = 16;
        if (nvs_get_str(nvs_handle, "mqtt_broker", &mqtt_broker[0], &required_size) != ESP_OK)
        {
            ESP_LOGE(TAG, "mqtt_broker not in NVS");
        }
        else
        {
            mqtt_cfg.uri = &mqtt_broker[0];
        }
        if (nvs_get_u8(nvs_handle, "mqtt_protocol", &mqtt_protocol) != ESP_OK)
        {
            ESP_LOGE(TAG, "mqtt_protocol not in NVS");
        }
        required_size = 16;
        if (nvs_get_str(nvs_handle, "mqtt_port", &mqtt_port[0], &required_size) != ESP_OK)
        {
            ESP_LOGE(TAG, "mqtt_port not in NVS");
        }
        nvs_close(nvs_handle);
    }
	
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
