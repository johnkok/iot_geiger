#ifndef __WIFI_H__
#define __WIFI_H__

#include "main.h"
#include "esp_mac.h"
#include "esp_wifi.h"

/* The examples use WiFi configuration that you can set via project configuration menu
 *
 *    If you'd rather not, just change the below entries to strings with
 *       the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
 *       */
#define EXAMPLE_ESP_WIFI_SSID      "IOT_GEIGER"
#define EXAMPLE_ESP_WIFI_PASS      "IOT_GEIGER"
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define EXAMPLE_ESP_WIFI_CHANNEL   0
#define EXAMPLE_MAX_STA_CONN       10

/* The event group allows multiple bits for each event, but we only care about two events:
 *  * - we are connected to the AP with an IP
 *   * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// functions
void wifi_init(void);

#endif
