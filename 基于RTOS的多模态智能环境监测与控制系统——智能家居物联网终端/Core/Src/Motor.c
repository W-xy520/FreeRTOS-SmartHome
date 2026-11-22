#include "Motor.h"
#include "main.h"
#include "tim.h"

void Set_Motor(int16_t PWM)
{
    if (PWM >= 0)
    {
        if (PWM > 100)
        {
            PWM = 100;
        }
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_11,GPIO_PIN_RESET);
        __HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_1,PWM);
    }
    else if (PWM < 0)
    {
        if (PWM < -100)
        {
            PWM = -100;
        }
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_11,GPIO_PIN_SET);
        __HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_1,-PWM);
    }
}

void LED_SetPWM(uint16_t PWM)
{
    __HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,PWM);
}
