#ifndef __AIR_QUALITY_H__
#define __AIR_QUALITY_H__

#include "main.h"
#include "driver/uart.h"

#define PM_TX_PIO 25
#define PM_RX_PIO 26

void air_quality_init(void);

#endif
