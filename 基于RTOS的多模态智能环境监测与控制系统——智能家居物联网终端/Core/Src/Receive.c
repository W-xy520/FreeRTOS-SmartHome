#include "stm32f1xx_hal.h"
#include "Receive.h"
#include "FreeRTOS_Demo.h"
#include "main.h"
#include "usart.h"
#include "ATCommand.h"
#include "semphr.h"

extern SemaphoreHandle_t ESP01S_Semaphore_handle;
extern SemaphoreHandle_t Voice_handle;

volatile uint16_t Rx_Count = 0;
uint8_t Rx = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

        Buffer_Write(&Rx,&AT_Buf);//每次都把接收到的数据写到环形缓冲区结构体中

        xSemaphoreGiveFromISR(ESP01S_Semaphore_handle,&pxHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
        
        HAL_UART_Receive_IT(&huart2, &Rx, 1);
    }

    if (huart->Instance == USART3)
    {
        BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

        Buffer_Write(&Rx,&AT_Buf);//每次都把接收到的数据写到环形缓冲区结构体中

        xSemaphoreGiveFromISR(Voice_handle,&pxHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
        
        HAL_UART_Receive_IT(&huart3, &Rx, 1);
    }

}
