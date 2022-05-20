#include "temp.h"
#include "DHT22.h"

static const char *TAG = "TEMP";

static void *temp_thread(void * arg)
{
    ESP_LOGI(TAG, "Thread started!"); 

    while (true)
    {

        if (readDHT() == DHT_OK)
        {
            ESP_LOGI(TAG, "Temp: %.2f Hum: %.2f", getTemperature(), getHumidity());
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
