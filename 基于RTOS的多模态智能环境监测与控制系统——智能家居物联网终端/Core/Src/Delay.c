#include "Delay.h"
#include "tim.h"

void Delay_us(uint32_t t)
{
    __HAL_TIM_SetCounter(&htim1,0);

    while(__HAL_TIM_GetCounter(&htim1) < t);
}

void Delay_ms(uint32_t t)
{
    uint32_t i = 0;

    for(i = 0;i < t;i ++)
    {
        Delay_us(1000);
    }
}

void Delay_s(uint32_t t)
{
    uint32_t i = 0;
    for(i = 0;i < t;i ++)
    {
        Delay_ms(1000);
    }
}
