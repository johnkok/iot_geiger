#include "geiger.h"

extern QueueHandle_t data_queue;

static const char *TAG = "GEIGER";
static unsigned int gc_event = 0;
float gc_hourly[GC_LOG_CNT];
short int hourly_index = 0;
static unsigned short int gc_event_ind = 0;
static unsigned char buzzer = 0;
static unsigned char led = 0;

void gc_int_handler(void *arg)
{
    gc_event++;
    gc_event_ind = 1;
}

static void *geiger_thread(void * arg)
{
    struct timeval tv_old, tv_new;
    unsigned char clear_event = 0;
    data_msg_s * gc_msg;

    for (int i = 0 ; i < GC_LOG_CNT ; i++)
    {
        gc_hourly[i] = 0.0;
    }

    gettimeofday(&tv_old, NULL);

    ESP_LOGI(TAG, "Thread started!");
    while (true)
    {
        usleep(10000);
        if (gc_event_ind)
        {
            if (buzzer > 0) gpio_set_level(BUZ_GPIO,0);
            if (led > 0) gpio_set_level(LED_GPIO,0);
            gc_event_ind = 0;
            clear_event = 1;
        }
        else if (clear_event != 0)
        {
            if (buzzer > 0) gpio_set_level(BUZ_GPIO,1);
            if (led > 0) gpio_set_level(LED_GPIO,1);
            clear_event = 0;
        }
        gettimeofday(&tv_new, NULL);
        if (tv_new.tv_sec >= (tv_old.tv_sec + 60))
        {
            gc_hourly[hourly_index] = gc_event * 0.00812037f; // Convertion factor for J305 tube
            gc_msg = malloc(sizeof(data_msg_s));
            if (gc_msg)
            {
                gc_msg->msg_src = GC_SRC;
                gc_msg->gc_usv = gc_hourly[hourly_index];
                if (xQueueSend( data_queue, (void *) &gc_msg, (TickType_t)0) != pdTRUE)
                {
                    free(gc_msg);
                }
            }
            tv_old = tv_new;
            hourly_index = (hourly_index + 1) % GC_LOG_CNT;
            gc_event = 0;
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
    nvs_handle_t nvs_handle;

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

    if (nvs_open_from_partition("iot_geiger", "default", NVS_READONLY, &nvs_handle) == ESP_OK)
    {
        if (nvs_get_u8(nvs_handle, "buzzer", &buzzer) != ESP_OK)
        {
            ESP_LOGE(TAG, "buzzer not in NVS");
        }
        if (nvs_get_u8(nvs_handle, "led", &led) != ESP_OK)
        {
            ESP_LOGE(TAG, "led not in NVS");
        }
    }

    // Clean log memory
    for (int i = 0 ; i < GC_LOG_CNT; i++)
    {
        gc_hourly[i] = 0.0;
    } 

    // Start Geiger counter thread
    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 1024*2);
    ret = pthread_create(&threadGeiger, &attr, geiger_thread, NULL);
    assert(ret == 0);
}
