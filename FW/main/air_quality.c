#include "air_quality.h"

static const char *TAG = "AIR";
const uart_port_t uart_pm = UART_NUM_2;

typedef struct air_quality_s {
    uint16_t cf1_1;    //!< CF=1, standard particle 1.0um
    uint16_t cf1_25;   //!< CF=1, standard particle 2.5um
    uint16_t cf1_10;   //!< CF=1, standard particle 10.0um
    uint16_t atm_1;    //!< PM 1.0 concentration [μg/m3]
    uint16_t atm_25;   //!< PM 2.5 concentration [μg/m3]
    uint16_t atm_100;  //!< PM 10.0 concentration [μg/m3]
    uint16_t p_03;     //!< number of particles with diameter beyond 0.3 um in 0.1 L of air
    uint16_t p_05;     //!< number of particles with diameter beyond 0.5 um in 0.1 L of air
    uint16_t p_10;     //!< number of particles with diameter beyond 1.0 um in 0.1 L of air
    uint16_t p_25;     //!< number of particles with diameter beyond 2.5 um in 0.1 L of air
    uint16_t p_50;     //!< number of particles with diameter beyond 5.0 um in 0.1 L of air
    uint16_t p_100;    //!< number of particles with diameter beyond 10.0 um in 0.1 L of air
 }air_quality_t;

uart_config_t uart_config = {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};

air_quality_t aq;

static void *air_thread(void * arg)
{
    int len = 0;
    uint16_t crc, plen;

    uint8_t * data = (uint8_t *) malloc(PM_BUFFER_SZ);

    ESP_LOGI(TAG, "Thread started!");

    while (data)
    {
        gpio_set_level(PM_SET,1);
        sleep(10);

        len = uart_read_bytes(uart_pm, data, PM_BUFFER_SZ, 20);

	if (len > 0) {
	    for (int x = 0 ;  x < (len-31) ; x++){
                if ((data[x] != 0x42) || (data[x+1] != 0x4D)) { // Valid Header "BM"
                    continue;
		}
                plen = data[x+3] + (data[x+2] << 8);
	        if (plen != 0x1C) { // 32 Bytes protocol for PMS7003
                    continue;
		}
                crc = 0;
		for (int y = 0 ; y < (plen + 2) ; y++) { // Calculate CRC
                    crc += data[x + y];
		}
		if (crc != ((uint16_t)(data[x+plen+2])<<8) + data[x+plen+3]) {
                    ESP_LOGI(TAG, "Error: CRC %X %X", crc, ((uint16_t)(data[x+plen+2])<<8) + data[x+plen+3]);
                    continue;
		}
                // CRC is valid
                memcpy(&aq, &data[x+4], sizeof(aq));
		ESP_LOGI(TAG, "PM1: %d, PM2.5: %d, PM10: %d", aq.atm_1, aq.atm_25, aq.atm_100);
	    }
	}
        gpio_set_level(PM_SET,0);
        sleep(290);
    }

    if (data != NULL)
    {
        free(data);
    }

    ESP_LOGE(TAG, "Thread ended!");
    return NULL;
}

void air_quality_init(void)
{
    pthread_attr_t attr;
    pthread_t threadAir;
    int ret;

    // Setup UART2 interface
    ESP_ERROR_CHECK(uart_param_config(uart_pm, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_pm, PM_TX_PIO, PM_RX_PIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_pm, PM_BUFFER_SZ, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_flush(uart_pm));

    // Set PM SET GPIOO
    gpio_config_t pm_conf;
    pm_conf.intr_type = GPIO_INTR_DISABLE;
    pm_conf.mode = GPIO_MODE_OUTPUT;
    pm_conf.pin_bit_mask = 1U<<PM_SET;
    pm_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    pm_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&pm_conf);

    gpio_set_level(PM_SET,0);

    // Start PWM thread
    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 1024*2);
    ret = pthread_create(&threadAir, &attr, air_thread, NULL);
    assert(ret == 0);
}
