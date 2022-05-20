#include "temp.h"

static const char *TAG = "TEMP";

static void *temp_thread(void * arg)
{
    ESP_LOGI(TAG, "Thread started!"); 
    while (true)
    {
        sleep(1);
    }
    ESP_LOGE(TAG, "Thread ended!");

    return NULL;
}

void temp_init(void)
{
    pthread_attr_t attr;
    pthread_t threadTemp;
    int ret;

    // Start PWM thread
    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 1024*2);
    ret = pthread_create(&threadTemp, &attr, temp_thread, NULL);
    assert(ret == 0);
}
