#include "air_quality.h"

static const char *TAG = "AIR";

static void *air_thread(void * arg)
{
	ESP_LOGI(TAG, "Thread started!");
    while (true)
    {
        sleep(1);
    }

	ESP_LOGE(TAG, "Thread ended!");
    return NULL;
}

void air_quality_init(void)
{
   pthread_attr_t attr;
   pthread_t threadAir;
   int ret;

   // Start PWM thread
   ret = pthread_attr_init(&attr);
   assert(ret == 0);
   pthread_attr_setstacksize(&attr, 10240);
   ret = pthread_create(&threadAir, &attr, air_thread, NULL);
   assert(ret == 0);
}
