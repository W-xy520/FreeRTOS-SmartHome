#ifndef __LED_H
#define __LED_H

#define LED1    GPIO_PIN_4
#define LED2    GPIO_PIN_5

#include "main.h"

void LED_ON(void);
void LED_OFF(void);
void LED_Turn(void);

#endif
