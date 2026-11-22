#include "Key.h"
#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "Serial.h"


uint8_t KeyNum = 0;
uint8_t pre_state = 0;
uint8_t cur_state = 0;


extern QueueHandle_t Key_Queue_handle;



uint8_t Key_GetState(void)
{
    if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12) == GPIO_PIN_RESET)
    {
        return 1;
    }
    else if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        return 2;
    }
    else if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_14) == GPIO_PIN_RESET)
    {
        return 3;
    }
    else if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_15) == GPIO_PIN_RESET)
    {
        return 4;
    }
    else if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_5) == GPIO_PIN_RESET)
    {
        return 5;
    }
    return 0;
}

void Key_Tick(void)
{
    static uint16_t Count = 0;
    Count ++;
    if (Count >= 1)
    {
        Count = 0;
        pre_state = cur_state;
        cur_state = Key_GetState();

        if (pre_state != cur_state)
        {
            KeyNum = pre_state;

            //往队列里写按键值
            xQueueSend(Key_Queue_handle,&KeyNum,0);
        }
    }
}

uint8_t Key_GetNum(void)
{
    uint8_t Temp = 0;
    Temp = KeyNum;
    KeyNum = 0;
    return Temp;
}
