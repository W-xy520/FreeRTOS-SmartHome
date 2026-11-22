#include "FreeRTOS_Demo.h"
/*FreeRTOS必须添加的库*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "main.h"
#include "LED.h"
#include "stdio.h"
#include "ATCommand.h"
#include "OLED.h"
#include "Sensor.h"
#include "Serial.h"
#include "Publish.h"
#include "Key.h"
#include "string.h"
#include "Motor.h"
#include "Humidifier.h"
#include "event_groups.h"
#include "Encoder.h"
#include "timers.h"
#include "Buzzer.h"
#include "iwdg.h"

/*一些按键功能的全局变量*/

volatile uint8_t LED_Flag = 0;
uint8_t LED_State = 0;

extern volatile uint16_t Servo_CCR;

// 创建一个事件标志组，接受完信号就发送到云端

extern QueueHandle_t Key_Queue_handle;

// 事件标志组句柄
extern EventGroupHandle_t Send_inf_handle;

// 蜂鸣器信号量句柄
SemaphoreHandle_t Buzzer_Semaphore_handle;

// 互斥量
QueueHandle_t WiFi_Mutex_handle;

// 串口接收数组
extern uint8_t AT_receive_buf[256];

// 任务句柄
TaskHandle_t Connect_wifi_handle;       // 连接wifi
TaskHandle_t AT_Task_Parser_handle;     // 监听任务
TaskHandle_t Init_handle;               // 初始化任务
TaskHandle_t Connect_emqx_handle;       // 连接EMQX平台
TaskHandle_t AT_SubscribeTopic_handle;  // 订阅主题任务
TaskHandle_t Switch_Control_handle;     // WiFi中的按键控制任务
TaskHandle_t Read_HumiTemp_handle;      // 读取温湿度
TaskHandle_t Read_AD_handle;            // 读取AD值
TaskHandle_t Key_Task_handle;           // 机械按键控制任务
TaskHandle_t OLED_Display_handle;       // OLED的显示任务
TaskHandle_t Send_inf_toEMQX_handle;    // 往WiFi上发送消息的任务
TaskHandle_t Encoder_Read_handle;       // 读取编码器数值的任务
TaskHandle_t Voice_Parser_handle;       // 语音监听任务
TaskHandle_t Buzzer_Parser_handle;      // 蜂鸣器
TaskHandle_t WiFi_EMQX_Listener_handle; // 网络监听

// 软件定时器句柄
TimerHandle_t Timer_handle;

// 任务
void AT_Task_Parser(void *param);
void Connect_wifi(void *param);
void Init(void *param);
void Connect_EMQX(void *param);
void AT_SubscribeTopic_Task(void *param);
void Switch_Control_Parser(void *param);
void Read_HumiTemp_Task(void *param);
void Read_AD_Task(void *param);
void Key_Task(void *param);
void OLED_Display_Task(void *param);
void Send_inf_toEMQX_Task(void *param);
void Encoder_Read_Task(void *param);
void Voice_Parser_Task(void *param);
void Buzzer_Parser_Task(void *param);
void WiFi_EMQX_Listener_Task(void *param);

EventGroupHandle_t EMQX_Event_handle; // 判断wifi和emqx是否在线的事件标志组的句柄



SemaphoreHandle_t ERROR_semaphore_handle;//发生错误时唤醒wifi监控



extern uint16_t ADC_Value[2];

float Relative_Light = 0;
float Relative_Air = 0;

char temperature[15] = {0};
char humidity[15] = {0};
char light[15] = {0};
char air[25] = {0};

// 软件定时器回调函数
void MyTimer_Callback(TimerHandle_t xTimer);

void vApplicationIdleHook(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

// 启动函数
void FreeRTOS_Start(void)
{
    // 创建初始化任务并且开启软件定时器
    xTaskCreate(Init, "Init", 100, NULL, 3, &Init_handle);
    Timer_handle = xTimerCreate("Soft_Timer", pdMS_TO_TICKS(20), pdTRUE, (void *)1, MyTimer_Callback);

    vTaskStartScheduler();
}

// 初始化并创建出来其他任务
void Init(void *param)
{
    // 互斥量
    WiFi_Mutex_handle = xSemaphoreCreateMutex();

    //创建信号量
    ERROR_semaphore_handle = xSemaphoreCreateBinary();

    // 创建蜂鸣器信号量
    Buzzer_Semaphore_handle = xSemaphoreCreateBinary();

    // 创建网络监听的事件标志组
    EMQX_Event_handle = xEventGroupCreate();

    xTaskCreate(Key_Task, "Key_Task", 130, NULL, 3, &Key_Task_handle);

    xTaskCreate(Connect_wifi, "Connect_wifi", 100, NULL, 1, &Connect_wifi_handle);
    xTaskCreate(AT_Task_Parser, "AT_Task_Parser", 150, NULL, 4, &AT_Task_Parser_handle);

    vTaskDelete(NULL);

    while (1)
        ;
}

// 连接WIFI的任务
void Connect_wifi(void *param)
{
    // 等待上电启动的信号处理完
    HAL_Delay(500);

    if (BKP->DR1 != 0xA5A5)
    {
        AT_SendCommand("AT");
        AT_SendCommand("AT+CWMODE=3");
        AT_SendCommand("AT+CWJAP=\"**********\",\"**********\"");
    }

    // 连接上WiFi后创建一个连接MQTT云端的任务
    xTaskCreate(Connect_EMQX, "Connect_EMQX", 100, NULL, 1, &Connect_emqx_handle);

    vTaskDelete(NULL);

    while (1)
        ;
}

// 连接MQTT平台的命令
void Connect_EMQX(void *param)
{
    if (BKP->DR1 != 0xA5A5)
    {
        // 连接前先初始化ESP8266
        AT_SendCommand("AT+MQTTCLEAN=0");

        // 连接EMQX
        AT_SendCommand("AT+MQTTUSERCFG=0,1,\"esp8266\",\"\",\"\",0,0,\"\""); // 连接公网，容易测试

        AT_SendCommand("AT+MQTTCONNCFG=0,30,0,\"esp/status\",\"offline\",0,1"); // 配置遗嘱

        AT_SendCommand("AT+MQTTCONN=0,\"broker.emqx.io\",1883,0");

        BKP->DR1 = 0xA5A5;
    }

    // 连接上云端后创建一个订阅主题的任务
    xTaskCreate(AT_SubscribeTopic_Task, "AT_SendandReceive_Task", 150, NULL, 1, &AT_SubscribeTopic_handle);

    vTaskDelete(NULL);
    while (1)
        ;
}

// 订阅主题的任务
void AT_SubscribeTopic_Task(void *param)
{
    // 订阅主题，后面可以接收这个主题的消息

    AT_SendCommand("AT+MQTTSUB=0,\"switch_control\",0"); // 订阅开关控制主题

    // 创建一个按键控制的监听任务
    xTaskCreate(Switch_Control_Parser, "Switch_Control_Parser", 80, NULL, 4, &Switch_Control_handle);
    // 创建读取温湿度和ADC的任务，还有按键控制的任务,还有把数据发送到云端的任务
    xTaskCreate(Read_HumiTemp_Task, "Read_HumiTemp_Task", 150, NULL, 2, &Read_HumiTemp_handle);
    xTaskCreate(Read_AD_Task, "Read_AD_Task", 135, NULL, 2, &Read_AD_handle);

    xTaskCreate(Send_inf_toEMQX_Task, "Send_inf_toEMQX_Task", 150, NULL, 2, &Send_inf_toEMQX_handle);
    // 读取编码器值的任务
    xTaskCreate(Encoder_Read_Task, "Encoder_Read_Task", 50, NULL, 3, &Encoder_Read_handle);

    // 创建一个语音模块的监听任务
    xTaskCreate(Voice_Parser_Task, "Voice_Parser_Task", 150, NULL, 4, &Voice_Parser_handle);

    // 创建一个监听蜂鸣器的任务
    xTaskCreate(Buzzer_Parser_Task, "Buzzer_Parser_Task", 50, NULL, 3, &Buzzer_Parser_handle);

    // 创建一个监听网络连接状况的任务
    xTaskCreate(WiFi_EMQX_Listener_Task, "WiFi_EMQX_Listener_Task", 180, NULL, 3, &WiFi_EMQX_Listener_handle);

    vTaskDelete(NULL);

    while (1)
        ;
}

void Read_HumiTemp_Task(void *param)
{
    while (1)
    {
        // 关闭任务切换，防止读取被打断
        Receive_Start();

        vTaskSuspendAll();

        Read_DHT11();

        xTaskResumeAll();

        xEventGroupSetBits(Send_inf_handle, TASK1_FLAG);

        // 获取栈高水位，调试用
        //  uint16_t res = uxTaskGetStackHighWaterMark2(Read_HumiTemp_handle);
        //  Serial1_Printf("Read_HumiTemp_Task:%d\r\n",res);

        // 调试的时候用这个
        //  Serial1_Printf("湿度：%d.%d%%,温度：%d.%d,校验和:%d\r\n",DHT11_DataBuf[0],DHT11_DataBuf[1],DHT11_DataBuf[2],DHT11_DataBuf[3],DHT11_DataBuf[4]);

        // 发送信息太浪费时间了，所以每次等两秒吧，不然影响OLED刷新
        vTaskDelay(2000);
    }
}

void Read_AD_Task(void *param)
{
    xTaskCreate(OLED_Display_Task, "OLED_Display_Task", 120, NULL, 1, &OLED_Display_handle);

    while (1)
    {
        Relative_Light = (float)100 - ADC_Value[0] / 4096.0 * 100;
        Relative_Air = ADC_Value[1] / 4096.0 * 100;

        // 处理数据
        light[6] = (uint8_t)Relative_Light / 10 % 10 + '0';
        light[7] = (uint8_t)Relative_Light % 10 + '0';
        light[9] = (uint16_t)(Relative_Light * 100) % 100 / 10 % 10 + '0';
        light[9] = (uint16_t)(Relative_Light * 100) % 10 + '0';

        air[13] = (uint8_t)Relative_Air / 10 % 10 + '0';
        air[14] = (uint8_t)Relative_Air % 10 + '0';
        air[16] = (uint16_t)(Relative_Air * 100) % 100 / 10 % 10 + '0';
        air[17] = (uint16_t)(Relative_Air * 100) % 10 + '0';

        xEventGroupSetBits(Send_inf_handle, TASK2_FLAG);

        if (Relative_Air >= 20) // 空气污染
        {
            xSemaphoreGive(Buzzer_Semaphore_handle);
        }

        // 获取栈高水位，调试用
        //  uint16_t res = uxTaskGetStackHighWaterMark2(Read_AD_handle);
        //  Serial1_Printf("Read_AD_Task:%d\r\n",res);

        // 调试的时候用这个
        //  Serial1_Printf("相对光照强度：%05.2f%%\r\n",Relative_Light);
        //  Serial1_Printf("相对空气污染：%05.2f%%\r\n",Realtive_Air);

        vTaskDelay(300);
    }
}

// 按键控制的任务
void Key_Task(void *param)
{
    uint8_t KeyNum = 0;

    // 开启软件定时器
    xTimerStart(Timer_handle, portMAX_DELAY);

    while (1)
    {
        xQueueReceive(Key_Queue_handle, &KeyNum, portMAX_DELAY);

        if (KeyNum == 1)
        {
            LED_Flag++;
            if (LED_Flag > 2)
            {
                LED_Flag = 0;
            }
            if (LED_Flag == 0)
            {
                LED_SetPWM(0);
            }
            else if (LED_Flag == 1)
            {
                LED_SetPWM(100);
            }
        }
        else if (KeyNum == 2)
        {
            Humidifier_Turn();
        }
        else if (KeyNum == 3)
        {
            Servo_CCR += 1;
            if (Servo_CCR > 2)
            {
                Servo_CCR = 1;
            }
        }
        else if (KeyNum == 4)
        {
            xSemaphoreTake(WiFi_Mutex_handle,portMAX_DELAY);

            // 重连EMQX
            AT_SendCommand("AT+MQTTCLEAN=0"); // 先把上次连接残留清理了

            AT_SendCommand("AT+MQTTUSERCFG=0,1,\"esp8266\",\"\",\"\",0,0,\"\"");

            AT_SendCommand("AT+MQTTCONNCFG=0,30,0,\"esp/status\",\"offline\",0,1");

            AT_SendCommand("AT+MQTTCONN=0,\"broker.emqx.io\",1883,0");

            AT_SendCommand("AT+MQTTSUB=0,\"switch_control\",0"); // 订阅开关控制主题

            xSemaphoreGive(WiFi_Mutex_handle);
        }
        else if (KeyNum == 5) // 编码器按键
        {
            // 关电机,按键按下把电机pwm置为0
            TIM4->CNT = 0;
            Set_Motor(0);
        }
    }
}

// OLED显示任务
void OLED_Display_Task(void *param)
{
    while (1)
    {
        OLED_Clear();
        OLED_ShowChinese(0, 0, "温度：");
        OLED_ShowChinese(0, 16, "湿度：");
        OLED_ShowChinese(0, 32, "光照强度：");
        OLED_ShowChinese(0, 48, "空气污染：");
        OLED_Printf(48, 0, OLED_8X16, "%d.%d", DHT11_DataBuf[2], DHT11_DataBuf[3]);
        OLED_Printf(48, 16, OLED_8X16, "%d%%", DHT11_DataBuf[0]);
        OLED_Printf(80, 32, OLED_8X16, "%05.2f%%", Relative_Light);
        OLED_Printf(80, 48, OLED_8X16, "%05.2f%%", Relative_Air);
        OLED_ShowChinese(80, 0, "℃");

        // OLED_Printf(0,0,OLED_8X16,"PWM:%d",Encoder_GetNum());

        // OLED_Printf(0,0,OLED_8X16,"CCR:%d",Servo_CCR);
        OLED_Update();

        vTaskDelay(300);

        // 获取栈高水位，调试用
        //  uint16_t res = uxTaskGetStackHighWaterMark2(OLED_Display_handle);
        //  Serial1_Printf("OLED_Display_Task:%d\r\n",res);

        // AT监听任务的栈高水位
        //  res = uxTaskGetStackHighWaterMark2(AT_Task_Parser_handle);
        //  Serial1_Printf("AT_Task_Parser_handle:%d\r\n",res);

        // 接收Wifi按键控制的栈高水位
        //  res = uxTaskGetStackHighWaterMark2(Switch_Control_handle);
        //  Serial1_Printf("Switch_Control_handle:%d\r\n",res);

        // 按键任务的栈高水位
        // uint16_t res = uxTaskGetStackHighWaterMark2(Key_Task_handle);
        // Serial1_Printf("Key_Task_handle:%d\r\n", res);
    }
}

// 往EMQX上发送信息的任务
void Send_inf_toEMQX_Task(void *param)
{
    while (1)
    {
        // 等待两个读取都完事了再发送
        xEventGroupWaitBits(Send_inf_handle, TASK1_FLAG | TASK2_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);

        snprintf(temperature, sizeof(temperature), "%.1fC", (float)DHT11_DataBuf[2] + (float)DHT11_DataBuf[3] / 10);
        snprintf(humidity, sizeof(humidity), "%d%%", DHT11_DataBuf[0]);
        snprintf(light, sizeof(light), "%05.2f%%", Relative_Light);
        snprintf(air, sizeof(air), "%05.2f%%", Relative_Air);

        if (xSemaphoreTake(WiFi_Mutex_handle, portMAX_DELAY) != pdFALSE)
        {
            // 没有被别的任务占用就发送信息
            Publish_Inf("Humi", humidity);
            Publish_Inf("Temp", temperature);
            Publish_Inf("Light", light);
            Publish_Inf("AirPollution", air);

            // 用完了释放
            xSemaphoreGive(WiFi_Mutex_handle);
        }

        // 查看栈高水位
        //  uint16_t res = uxTaskGetStackHighWaterMark2(Send_inf_toEMQX_handle);
        //  Serial1_Printf("Send_inf_toEMQX_handle:%d\r\n",res);
    }
}

extern QueueHandle_t Encoder_handle;

// 编码器
void Encoder_Read_Task(void *param)
{
    while (1)
    {
        xQueueSemaphoreTake(Encoder_handle, portMAX_DELAY);

        Set_Motor(Encoder_GetNum());
    }
}

// 软件定时器回调函数功能
void MyTimer_Callback(TimerHandle_t xTimer) // 20ms进行一次
{
    if (xTimer == Timer_handle)
    {
        Key_Tick();

        static uint16_t Count = 0;
        Count++;
        if (Count >= 2) // 每四十毫秒看看有没有拧编码器
        {
            Count = 0;
            if (Encoder_GetNum() != 0)
            {
                BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(Encoder_handle, &pxHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
            }
        }

        static uint8_t LED_PWM = 100;
        if (LED_Flag == 2)
        {
            if (LED_State == 1)
            {
                LED_SetPWM(LED_PWM);
                LED_PWM++;
                if (LED_PWM >= 100)
                {
                    LED_PWM = 100;
                    LED_State = 0;
                }
            }
            else
            {
                LED_SetPWM(LED_PWM);
                LED_PWM--;
                if (LED_PWM == 0)
                {
                    LED_State = 1;
                }
            }
        }
    }
}

// 蜂鸣器监听任务
void Buzzer_Parser_Task(void *parazm)
{
    while (1)
    {
        xSemaphoreTake(Buzzer_Semaphore_handle, portMAX_DELAY);

        Buzzer_ON();
        vTaskDelay(300);
        Buzzer_OFF();
        vTaskDelay(700);
    }
}

// 网络监听任务掉线了自动重连
void WiFi_EMQX_Listener_Task(void *param)
{
    uint8_t wifi_connected; // 连接状态
    uint8_t mqtt_connected; // 连接状态
    uint16_t rx_start;      // 开始接收的位置
    uint16_t rx_end;        // 结束位置
    uint16_t rx_length;
    uint16_t i;
    uint8_t response[150]; // 接收回复的数组

    while (1)
    {
        /*等到错误了之后去检查*/
        xSemaphoreTake(ERROR_semaphore_handle,portMAX_DELAY);

        if (xSemaphoreTake(WiFi_Mutex_handle, portMAX_DELAY) != pdFALSE)
        {
            // 防止跟发送消息的的任务冲突，两个同一时间只能有一个运行

            rx_start = AT_Buf.write;
            AT_SendCommand("AT+CWSTATE?");
            rx_end = AT_Buf.write;
            rx_length = (rx_end >= rx_start)
                            ? (uint16_t)(rx_end - rx_start)
                            : (uint16_t)(sizeof(AT_Buf.Buffer) - rx_start + rx_end);
            if (rx_length >= sizeof(response))
            {
                rx_length = sizeof(response) - 1; // 留一个空放'\0'
            }
            for (i = 0; i < rx_length; i++)
            {
                response[i] = AT_Buf.Buffer[(rx_start + i) % sizeof(AT_Buf.Buffer)];
            }
            response[rx_length] = '\0';
            Serial1_Printf("%s\r\n", response);

            wifi_connected = (strstr((const char *)response, "+CWSTATE:2,") != NULL);
            if (wifi_connected)
            {
                // WiFi在线
                Serial1_Printf("WiFi在线\r\n");
            }
            else
            {
                Serial1_Printf("WiFi掉线了\r\n");
                AT_SendCommand("AT+CWJAP=\"**********\",\"**********\""); // 重连
                Serial1_Printf("WiFi已重连\r\n");
            }

            rx_start = AT_Buf.write;
            AT_SendCommand("AT+MQTTCONN?");
            rx_end = AT_Buf.write;
            rx_length = (rx_end >= rx_start)
                            ? (uint16_t)(rx_end - rx_start)
                            : (uint16_t)(sizeof(AT_Buf.Buffer) - rx_start + rx_end);
            if (rx_length >= sizeof(response))
            {
                rx_length = sizeof(response) - 1;
            }
            for (i = 0; i < rx_length; i++)
            {
                response[i] = AT_Buf.Buffer[(rx_start + i) % sizeof(AT_Buf.Buffer)];
            }
            response[rx_length] = '\0';
            Serial1_Printf("%s\r\n", response);

            /* MQTT 状态值为 1 表示已连接 EMQX。 */
            mqtt_connected = (strstr((const char *)response, "+MQTTCONN:0,4") != NULL ||
                              strstr((const char *)response, "+MQTTCONN:0,5") != NULL ||
                              strstr((const char *)response, "+MQTTCONN:0,6") != NULL);
            if (mqtt_connected)
            {
                Serial1_Printf("EMQX在线\r\n");
            }
            else
            {
                Serial1_Printf("EMQX掉线了\r\n");
                if (wifi_connected)
                {
                    uint8_t KeyNum = 4;
                    xQueueSend(Key_Queue_handle, &KeyNum, 0);
                    Serial1_Printf("EMQX已重连\r\n");
                }
            }

            //  uint16_t res = uxTaskGetStackHighWaterMark2(WiFi_EMQX_Listener_handle);
            //  Serial1_Printf("WiFi_EMQX_Listener_Task:%d\r\n",res);

            // 用完了释放
            xSemaphoreGive(WiFi_Mutex_handle);
        }
    }
}
