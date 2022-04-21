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

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

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
	
    // All done, exit?
    while (true){
		sleep(10.0);
	}
}
