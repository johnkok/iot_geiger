/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "main.h"
#include "display.h"
#include "wifi.h"
#include "pwm.h"
#include "web_server.h"
#include "mqtt_cli.h"
#include "geiger.h"
#include "temp.h"
#include "air_quality.h"

static const char *TAG = "MAIN";
QueueHandle_t data_queue;
rx_store_s rx_store = {};

void app_main(void)
{
    data_msg_s * data_msg_rx;
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Initialize custom NVS
    ret = nvs_flash_init_partition("iot_geiger");
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase_partition("iot_geiger"));
        ret = nvs_flash_init_partition("iot_geiger");
    }
    ESP_ERROR_CHECK(ret);

    // Create data queue
    data_queue = xQueueCreate( 16, sizeof( struct data_message * ) );;

    // Init display
    display_init();

    // Prepare web server - before connecting wifi
    web_init();

    // Start wifi
    wifi_init();

    // Init power supply
    pwm_init();

    // Init sensors
    geiger_init();
    temp_init();
    air_quality_init();

    // Init web services
    mqtt_init();

    ESP_LOGI(TAG, "Initialization finished!");
    rx_store.current.temperature = 0.0f;
    rx_store.current.humidity = 0.0f;
    rx_store.current.gc_usv = 0.0f;

    // All done, idle looP
    while (true)
    {
        if (data_queue != 0)
        {
            if(xQueueReceive(data_queue, &data_msg_rx, (TickType_t)10000))
            {
                switch (data_msg_rx->msg_src)
                {
                case (GC_SRC):
                    rx_store.current.gc_usv = data_msg_rx->gc_usv;
                    break;
                case (DHT_SRC):
                    rx_store.current.temperature = data_msg_rx->dht_info.temperature;
                    rx_store.current.humidity = data_msg_rx->dht_info.humidity;
                    break;
                case (PM_SRC):
                    rx_store.current.pm1 = data_msg_rx->pm_info.pm1;
                    rx_store.current.pm2_5 = data_msg_rx->pm_info.pm2_5;
                    rx_store.current.pm10 = data_msg_rx->pm_info.pm10;
                    break;
                default:
                    ESP_LOGE(TAG, "Invalid message type (%X)!", data_msg_rx->msg_src);
                    break;
                }
                free(data_msg_rx);
            }
        }
        else // Should not be here...
        {
            ESP_LOGE(TAG, "Queue initialization error!");
            break;
        }
    }
}
