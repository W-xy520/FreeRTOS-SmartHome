#ifndef __FREERTOS_DEMO_H
#define __FREERTOS_DEMO_H

#include "stdint.h"

void FreeRTOS_Start(void);

#define TASK1_FLAG         0x01 << 0
#define TASK2_FLAG       0x01 << 1

extern volatile uint8_t heartbeat_listener_Flag;

#endif
