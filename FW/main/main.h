#ifndef __MAIN_H__
#define __MAIN_H__

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "pthread.h"
#include "esp_pthread.h"

#include "lwip/err.h"
#include "lwip/sys.h"

typedef enum msg_src_t
{
    GC_SRC,
    DHT_SRC,
    PM_SRC
}msg_src_e;

typedef struct pm_info_t
{
    unsigned short pm1;
    unsigned short pm2_5;
    unsigned short pm10;
}pm_info_s;

typedef struct dht_info_t
{
    float temperature;
    float humidity;
}dht_info_s;

typedef struct rx_data_t
{
    unsigned short pm1;
    unsigned short pm2_5;
    unsigned short pm10;
    float temperature;
    float humidity;
    float gc_usv;
}rx_data_s;

typedef struct rx_store_t
{
    rx_data_s current;
//    rx_data_s mean;
//    rx_data_s hourly[24];
}rx_store_s;

typedef struct data_msg_t
{
    msg_src_e msg_src;
    union {
        float gc_usv;
        pm_info_s pm_info;
        dht_info_s dht_info;
    };
} data_msg_s;

#endif
