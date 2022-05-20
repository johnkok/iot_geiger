#ifndef __GEIGER_H__
#define __GEIGER_H__

#include "main.h"
#include "driver/gpio.h"

#define LED_GPIO 14
#define GC_GPIO 13
#define BUZ_GPIO 12

void geiger_init(void);

#endif
