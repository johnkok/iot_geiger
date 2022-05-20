#include "geiger.h"

extern QueueHandle_t data_queue;

static const char *TAG = "GEIGER";
static unsigned short int gc_event = 0;

void gc_int_handler(void *arg)
{
    data_msg_s * gc_msg;

    gc_msg = malloc(sizeof(data_msg_s));
    if (gc_msg)
    {
        gc_event++;
        gc_msg->msg_src = GC_SRC;
        gc_msg->gc_count = gc_event;
        if (xQueueSend( data_queue, (void *) &gc_msg, (TickType_t)0) != pdTRUE)
        {
            free(gc_msg);
        }
    }
}

static void *geiger_thread(void * arg)
{
    unsigned char clear_event = 0;

    ESP_LOGI(TAG, "Thread started!");
    while (true)
    {
        usleep(10000);
        if (gc_event)
        {
            gpio_set_level(BUZ_GPIO,0);
            gpio_set_level(LED_GPIO,0);
            gc_event--;
            clear_event = 1;
        }
        else if (clear_event != 0)
        {
            gpio_set_level(BUZ_GPIO,1);
            gpio_set_level(LED_GPIO,1);
            clear_event = 0;
        }
    }
    ESP_LOGE(TAG, "Thread ended!");

    return NULL;
}

void geiger_init(void)
{
    pthread_attr_t attr;
    pthread_t threadGeiger;
    int ret;

    // Confidure BUZZER GPIO
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1U<<BUZ_GPIO;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(BUZ_GPIO,1);

    // Confidure LED GPIO
    io_conf.pin_bit_mask = 1U<<LED_GPIO;
    gpio_config(&io_conf);

    // Confidure Geiger counter interrupt line
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = 1U<<GC_GPIO;
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GC_GPIO, gc_int_handler, NULL);

    // Start Geiger counter thread
    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 1024*2);
    ret = pthread_create(&threadGeiger, &attr, geiger_thread, NULL);
    assert(ret == 0);
}
