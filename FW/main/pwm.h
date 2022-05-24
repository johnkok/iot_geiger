#ifndef __MQTT_H__
#define __MQTT_H__

#include "main.h"
#include "driver/ledc.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_OUTPUT_IO          (32) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_11_BIT // Set duty resolution to 11 bits
#define LEDC_DUTY               (500) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 4 kHz

// functions
void pwm_init(void);

#endif
