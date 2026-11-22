#include "Buzzer.h"
#include "main.h"


void Buzzer_ON(void)
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
}

void Buzzer_OFF(void)
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
}

void Buzzer_Turn(void)
{
    if (HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_12) == GPIO_PIN_RESET)
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
    }
    else 
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_RESET);
    }
}

