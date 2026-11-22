#include "Humidifier.h"
#include "main.h"

void Humidifier_OFF(void)
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
}

void Humidifier_ON(void)
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
}

void Humidifier_Turn(void)
{
    if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1) == SET)
    {
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
    }
    
}
