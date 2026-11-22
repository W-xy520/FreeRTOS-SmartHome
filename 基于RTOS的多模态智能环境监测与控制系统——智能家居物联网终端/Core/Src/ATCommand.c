#include "ATCommand.h"
#include "FreeRTOS_Demo.h"
#include "Serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "string.h"
#include "Serial.h"
#include "LED.h"
#include "Humidifier.h"
#include "Motor.h"
#include "queue.h"
#include "event_groups.h"

#define BUFFER_MAX 256

// 信号量句柄
extern QueueHandle_t ESP01S_Semaphore_handle;
extern QueueHandle_t AT_SendResult_handle;
extern QueueHandle_t switch_control_handle;
extern SemaphoreHandle_t Voice_handle;

extern SemaphoreHandle_t Buzzer_Semaphore_handle;


extern SemaphoreHandle_t ERROR_semaphore_handle;



extern volatile uint8_t LED_Flag;

extern volatile uint32_t heartbeat_timeout;

// 按键的队列
extern QueueHandle_t Key_Queue_handle;


//WiFi和EMQX检测的句柄
extern EventGroupHandle_t EMQX_Event_handle;



Sbuf AT_Buf; // 环形缓冲区结构体，内部存的是读写的标志位语音模块也用的这个缓冲区

uint8_t AT_receive_buf[BUFFER_MAX]; // 用来接收数据
uint8_t Switch_receive_buf[100];    // 用来接收按键控制的数据
uint8_t Voice_receive_buf[100];     // 用来接收语音模块的信息

void ATCommand_Init(void)
{
    AT_Buf.read = 0;
    AT_Buf.write = 0;
}

uint8_t AT_SendCommand(char *string)
{
    uint8_t res = 0;

    Serial2_Printf("%s\r\n", string);

    res = xSemaphoreTake(AT_SendResult_handle, 3500); // 每次最多等3.5秒

    return res;
}

// 读写的函数
void Buffer_Write(uint8_t *w, Sbuf *Buf) // Buf是那个结构体，里面存的是标志位，不是数据
{
    uint16_t i = (Buf->write + 1) % BUFFER_MAX; // 宏定义，就是写的数组最大值

    if (i != Buf->read)
    {
        Buf->Buffer[Buf->write] = *w;
        Buf->write = i;
    }
}

uint8_t Buffer_Read(uint8_t *r, Sbuf *Buf)
{
    if (Buf->read == Buf->write)
    {
        return 0;
    }
    else
    {
        *r = Buf->Buffer[Buf->read];
        Buf->read = (Buf->read + 1) % BUFFER_MAX;
        return 1;
    }
}

uint8_t Switch_Control_Read(uint8_t *r, Sbuf *Buf) // 第一个数组接收前置命令，第二个数组接收按键控制的正式命令
{
    uint16_t i = 0;
    uint16_t len = 0;
    uint8_t status = 0;

    while (1)
    {
        xSemaphoreTake(ESP01S_Semaphore_handle, portMAX_DELAY);

        Buffer_Read(&r[i], Buf);
        if (r[i] >= '0' && r[i] <= '9')
        {
            len = len * 10 + r[i] - '0';
            status = 1;
        }
        else if (r[i] == ',' && status == 1)
        {
            i++;
            break;
        }
        else if (r[i] == '\n' && r[i - 1] == '\r')
        {
            return 0;
        }
        i++;
    }

    while (1)
    {
        xSemaphoreTake(ESP01S_Semaphore_handle, portMAX_DELAY);

        Buffer_Read(&r[i], Buf);
        if (r[i] == '\n' && r[i - 1] == '\r')
        {
            r[i + 1] = '\0';
            xSemaphoreGive(switch_control_handle);
            i = 0;
            return len;
        }
        i++;
    }
}

// 检测AT命令是否发送成功的监听任务
void AT_Task_Parser(void *param)
{
    uint16_t i = 0;

    while (1)
    {
        xSemaphoreTake(ESP01S_Semaphore_handle, portMAX_DELAY);

        Buffer_Read(&AT_receive_buf[i], &AT_Buf);
        AT_receive_buf[i + 1] = '\0';

        // 如果先接收到回车就说明没有接收到特殊信号（订阅的信号）
        if (i > 0 && AT_receive_buf[i] == '\n' && AT_receive_buf[i - 1] == '\r')
        {
            if (strstr((const char *)AT_receive_buf, "OK\r\n") != NULL)
            {
                i = 0;
                // Serial1_Printf("%s\r\n", AT_receive_buf);
                xSemaphoreGive(AT_SendResult_handle);
            }
            else if (strstr((const char *)AT_receive_buf, "ERROR\r\n") != NULL)
            {
                i = 0;
                // Serial1_Printf("%s\r\n", AT_receive_buf);

                xSemaphoreGive(ERROR_semaphore_handle);

                xSemaphoreGive(AT_SendResult_handle);
            }
            else
            {
                i = 0;
                // Serial1_Printf("%s\r\n", AT_receive_buf);
            }
        }
        else if (strstr((const char *)AT_receive_buf, "switch_control") != NULL)
        {
            i = 0;

            uint16_t len = Switch_Control_Read(Switch_receive_buf, &AT_Buf);
            Serial1_Printf("接收到了%d个字节数据，他们是%s\r\n", len, &Switch_receive_buf[4]);
        }
        else
        {
            i++;
        }
    }
}

// 按键控制的监听任务
void Switch_Control_Parser(void *param)
{
    while (1)
    {
        xSemaphoreTake(switch_control_handle, portMAX_DELAY);

        if (strstr((const char *)Switch_receive_buf, "LED_ON\r\n") != NULL)
        {
            LED_SetPWM(100);
        }
        else if (strstr((const char *)Switch_receive_buf, "LED_OFF\r\n") != NULL)
        {
            LED_SetPWM(0);
            LED_Flag = 0;
        }
        else if (strstr((const char *)Switch_receive_buf, "Humidifier_ON\r\n") != NULL)
        {
            Humidifier_ON();
        }
        else if (strstr((const char *)Switch_receive_buf, "Humidifier_OFF\r\n") != NULL)
        {
            Humidifier_OFF();
        }
        else if (strstr((const char *)Switch_receive_buf, "FAN_1\r\n") != NULL)
        {
            TIM4->CNT = 35;
        }
        else if (strstr((const char *)Switch_receive_buf, "FAN_2\r\n") != NULL)
        {
            TIM4->CNT = 65;
        }
        else if (strstr((const char *)Switch_receive_buf, "FAN_3\r\n") != NULL)
        {
            TIM4->CNT = 100;
        }
        else if (strstr((const char *)Switch_receive_buf, "FAN_OFF\r\n") != NULL)
        {
            TIM4->CNT = 0;
            Set_Motor(0);
        }
        else if (strstr((const char *)Switch_receive_buf, "LED_Breathe\r\n") != NULL)
        {
            LED_Flag = 2;
        }
        else if (strstr((const char *)Switch_receive_buf, "Buzzer\r\n") != NULL)
        {
            xSemaphoreGive(Buzzer_Semaphore_handle);
        }
    }
}

// 语音的监听任务
void Voice_Parser_Task(void *param)
{
    uint16_t i = 0;

    while (1)
    {
        xSemaphoreTake(Voice_handle, portMAX_DELAY);

        Buffer_Read(&Voice_receive_buf[i], &AT_Buf);
        Voice_receive_buf[i + 1] = '\0';

        if (i > 0 && Voice_receive_buf[i] == '\n' && Voice_receive_buf[i - 1] == '\r')
        {
            if (strstr((const char *)Voice_receive_buf, "LED_ON\r\n") != NULL)
            {
                LED_SetPWM(100);
            }
            else if (strstr((const char *)Voice_receive_buf, "LED_OFF\r\n") != NULL)
            {
                LED_SetPWM(0);
                LED_Flag = 0;
            }
            else if (strstr((const char *)Voice_receive_buf, "Humidifier_ON\r\n") != NULL)
            {
                Humidifier_ON();
            }
            else if (strstr((const char *)Voice_receive_buf, "Humidifier_OFF\r\n") != NULL)
            {
                Humidifier_OFF();
            }
            else if (strstr((const char *)Voice_receive_buf, "FAN_1\r\n") != NULL)
            {
                TIM4->CNT = 35;
            }
            else if (strstr((const char *)Voice_receive_buf, "FAN_2\r\n") != NULL)
            {
                TIM4->CNT = 65;
            }
            else if (strstr((const char *)Voice_receive_buf, "FAN_3\r\n") != NULL)
            {
                TIM4->CNT = 100;
            }
            else if (strstr((const char *)Voice_receive_buf, "FAN_OFF\r\n") != NULL)
            {
                TIM4->CNT = 0;
                Set_Motor(0);
            }
            else if (strstr((const char *)Voice_receive_buf, "EMQX_Connect\r\n") != NULL)
            {
                uint8_t KeyNum = 4;

                xQueueSend(Key_Queue_handle, &KeyNum, 1000); // 最多等一秒
            }
            else if (strstr((const char *)Voice_receive_buf, "LED_Breathe\r\n") != NULL)
            {
                LED_Flag = 2;
            }

            i = 0;
        }
        else
        {
            i++;
        }
    }
}
