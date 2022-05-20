#include "temp.h"
#include "DHT22.h"

extern QueueHandle_t data_queue;

static const char *TAG = "TEMP";

static void *temp_thread(void * arg)
{
    data_msg_s * dht_msg;

    ESP_LOGI(TAG, "Thread started!"); 

    while (true)
    {

        if (readDHT() == DHT_OK)
        {
            ESP_LOGI(TAG, "Temp: %.2f Hum: %.2f", getTemperature(), getHumidity());

            dht_msg = malloc(sizeof(data_msg_s));
            if (dht_msg)
            {
                dht_msg->msg_src = DHT_SRC;
                dht_msg->dht_info.temperature = getTemperature();
                dht_msg->dht_info.humidity = getHumidity();
                if (xQueueSend(data_queue, (void *) &dht_msg, (TickType_t)0) != pdTRUE)
                {
                    free(dht_msg);
                }
            }
        }
        sleep(60);
    }
    ESP_LOGE(TAG, "Thread ended!");

    return NULL;
}

void temp_init(void)
{
    pthread_attr_t attr;
    pthread_t threadTemp;
    int ret;

    setDHTgpio(DHT22_PIN);

    // Start PWM thread
    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 1024*2);
    ret = pthread_create(&threadTemp, &attr, temp_thread, NULL);
    assert(ret == 0);
}
