#ifndef __AIR_QUALITY_H__
#define __AIR_QUALITY_H__

#include "main.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define PM_TX_PIO 22
#define PM_RX_PIO 19
#define PM_SET    18

#define PM_BUFFER_SZ 512

void air_quality_init(void);

#endif
