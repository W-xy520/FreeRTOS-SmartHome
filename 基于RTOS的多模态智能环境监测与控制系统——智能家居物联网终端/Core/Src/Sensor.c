#include "main.h"
#include "Sensor.h"
#include "Delay.h"
#include "tim.h"
#include "Serial.h"
#include "OLED.h"

uint32_t TimeOut = 0;
uint8_t DHT11_DataBuf[10] = {0};
uint8_t GetData = 0x00;


void Receive_Start(void)
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);
    HAL_Delay(18);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET);
	Delay_us(20);
}

uint8_t Receive_ACK(void)
{
    TimeOut = HAL_GetTick();
    while((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == SET) && (HAL_GetTick() - TimeOut < 300));
    if (HAL_GetTick() - TimeOut > 200)
    {
        return 1;
    }
    return 0;
}

void Receive_Data(void)
{
	for(uint8_t i = 0;i < 5;i ++)
	{
		DHT11_DataBuf[i + 5] = DHT11_DataBuf[i];//把原来的存起来，要是校验和不对就还用原来的数据
	}
	
    TimeOut = HAL_GetTick();
    //这是接收应答的时序，还没结束
    while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == RESET && HAL_GetTick() - TimeOut < 500);
    while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == SET && HAL_GetTick() - TimeOut < 500);

    //开始接收数据
    for(uint8_t j = 0;j < 5;j ++)
    {
        for (uint8_t i = 0;i < 8;i ++)
        { 
            while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == RESET && HAL_GetTick() - TimeOut < 500);
            __HAL_TIM_SetCounter(&htim1,0);
            while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9) == SET && HAL_GetTick() - TimeOut < 500);
            if (__HAL_TIM_GetCounter(&htim1) > 50)
            {
                GetData |= 0x01 << (7-i);
            }
        }
        DHT11_DataBuf[j] = GetData;
        GetData = 0x00;
    }
}

uint8_t Read_DHT11(void)
{
    if (Receive_ACK() == 1)
    {
		Serial1_Printf("未应答\r\n");//调试信息
        return 0;
    }
    Receive_Data();

    if ((uint8_t)(DHT11_DataBuf[0] + DHT11_DataBuf[1] + DHT11_DataBuf[2] + DHT11_DataBuf[3]) != DHT11_DataBuf[4])
    {
		for(uint8_t i = 0;i < 5;i ++)
		{
            Serial1_Printf("校验和出错，保留上一次的数据\r\n");
			DHT11_DataBuf[i] = DHT11_DataBuf[i + 5];
		}
        return 0;
    }
	return 1;
}
