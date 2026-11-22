#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

uint8_t Read_DHT11(void);
extern uint8_t DHT11_DataBuf[10];
void Receive_Start(void);

#endif
