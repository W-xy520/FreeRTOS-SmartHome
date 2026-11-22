#include "Encoder.h"
#include "main.h"
#include "gpio.h"
#include "tim.h"

int16_t Encoder_Num = 0;

int16_t Encoder_GetNum(void)
{   
    Encoder_Num = __HAL_TIM_GetCounter(&htim4);

    return Encoder_Num;
}
