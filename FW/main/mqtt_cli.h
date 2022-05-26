#ifndef __MQTT_CLI_H__
#define __MQTT_CLI_H__

#include "main.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"


void mqtt_init(void);
void mqtt_publish_data(esp_mqtt_client_handle_t client);

#endif
