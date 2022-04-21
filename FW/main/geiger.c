#include "geiger.h"

static const char *TAG = "GEIGER";

static void *geiger_thread(void * arg)
{
	ESP_LOGI(TAG, "Thread started!");
    while (true)
    {
        sleep(1);
    }
	ESP_LOGE(TAG, "Thread ended!");

    return NULL;
}

void geiger_init(void)
{
    pthread_attr_t attr;
    pthread_t threadGeiger;
    int ret;

    // Start PWM thread
    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 10240);
    ret = pthread_create(&threadGeiger, &attr, geiger_thread, NULL);
    assert(ret == 0);
}
