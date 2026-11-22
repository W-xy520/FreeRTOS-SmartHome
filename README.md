# FreeRTOS-SmartHome

**基于 RTOS 的多模态智能环境监测与控制系统 —— 智能家居物联网终端**

![MCU](https://img.shields.io/badge/MCU-STM32F103-0b5394)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-1f6feb)
![Protocol](https://img.shields.io/badge/Protocol-MQTT-3fb950)
![IDE](https://img.shields.io/badge/IDE-Keil%20MDK-8957e5)
![License](https://img.shields.io/badge/License-MIT-yellow)

以 **STM32F103** + **FreeRTOS** 为核心，通过 DHT11、光敏电阻、空气质量传感器采集环境数据，本地 OLED 实时显示，并通过 ESP-01S（ESP8266）以 AT 指令接入 WiFi，走 **MQTT** 把数据上报到云平台，同时接收云端下发的控制指令，实现"环境感知 + 本地交互 + 远程控制"的智能家居终端。

---

## 功能特性

| 模块 | 说明 |
| --- | --- |
| 环境采集 | DHT11 采集温湿度；ADC 采样光敏电阻（光照强度）与空气质量传感器 |
| 本地显示 | 0.96 寸 OLED 实时显示温度、湿度、光照强度、空气污染度 |
| 本地交互 | 5 个按键分别控制 LED 亮度模式、加湿器开关、舵机角度、MQTT 重连、电机急停 |
| 执行器 | LED PWM 调光（含呼吸灯模式）、直流电机 PWM 调速 + 旋转编码器闭环、加湿器继电器、舵机（软件 PWM）、蜂鸣器报警 |
| 云端通信 | ESP-01S 通过 AT 指令连接 WiFi 与 EMQX，上报环境数据、订阅控制主题 |
| 可靠性 | 独立看门狗（IWDG）喂狗、WiFi/MQTT 掉线检测与自动重连、互斥量保护串口共享 |

空气质量超过阈值（默认 20%）时自动触发蜂鸣器间歇报警。

## 软件架构

RTOS 任务划分（优先级由高到低按创建顺序）：

| 任务 | 职责 |
| --- | --- |
| `AT_Task_Parser` | 解析 ESP-01S 的 AT 应答，按命令顺序释放信号量 |
| `Switch_Control_Parser` | 解析云端下发的 `switch_control` 主题，执行开关动作 |
| `Voice_Parser_Task` | 解析语音识别模块串口数据，映射为控制指令 |
| `Connect_wifi` / `Connect_EMQX` / `AT_SubscribeTopic_Task` | 联网、连接 MQTT、订阅主题（启动阶段链式创建） |
| `Read_HumiTemp_Task` | 每 2s 读取 DHT11 |
| `Read_AD_Task` | 每 300ms 采样光照与空气质量，创建 OLED 显示任务 |
| `Key_Task` | 按键队列消费，处理本地控制 |
| `Encoder_Read_Task` | 编码器脉冲 → 电机目标速度 |
| `Send_inf_toEMQX_Task` | 事件组同步温湿度与 ADC 数据齐备后批量上报 |
| `Buzzer_Parser_Task` | 报警鸣叫 |
| `WiFi_EMQX_Listener_Task` | 周期性查询 WiFi / MQTT 状态，掉线自动重连 |
| `OLED_Display_Task` | 界面刷新 |

任务间通信：二值/计数信号量、互斥量（WiFi 串口共享）、消息队列（按键、编码器）、事件组（数据就绪同步）、软件定时器（20ms 按键扫描与呼吸灯）。

## 硬件引脚

| 引脚 | 复用/模式 | 用途 |
| --- | --- | --- |
| PA0 / PA1 | ADC1_IN0 / IN1 | 光敏电阻 / 空气质量传感器 |
| PA2 / PA3 | USART2 | ESP-01S（ESP8266）AT 指令 |
| PA9 / PA10 | USART1 | 调试串口（printf 输出） |
| PB10 / PB11 | USART3 | 语音识别模块 |
| PA4 / PA5 | GPIO 推挽输出 | OLED 软件 I2C（SCL / SDA） |
| PB9 | GPIO | DHT11 单总线 |
| PB5、PB12~PB15 | GPIO 输入 | 5 个独立按键 |
| PB6 / PB7 | TIM4_CH1 / CH2 | 旋转编码器 A/B 相 |
| PA6 / PB0 | TIM3 PWM | 电机调速 / LED 调光 |
| PA8 / PA11 | GPIO 输出 | 电机方向控制 |
| PA7 | GPIO 输出 | 舵机信号（TIM2 中断软件 PWM） |
| PB1 | GPIO 输出 | 加湿器继电器 |
| PA12 | GPIO 输出 | 蜂鸣器 |
| PC14 / PC15 | LSE | RTC 外部低速晶振 |

## MQTT 通信

- Broker：`broker.emqx.io:1883`（公共 EMQX 测试服务器）
- 上报主题：`Humi`、`Temp`、`Light`、`AirPollution`
- 订阅主题：`switch_control`
- 遗嘱消息：`esp/status` → `offline`

## 编译与运行

1. 使用 **Keil MDK 5** 打开 `基于RTOS的多模态智能环境监测与控制系统——智能家居物联网终端/MDK-ARM/Key.uvprojx`
2. 编译前请修改 `Core/Src/FreeRTOS_Demo.c` 中的 WiFi 账号密码（仓库中已脱敏为 `**********`）
3. 编译烧录：ST-Link 连接 SWD 接口，下载 `Key.hex`
4. 首次上电自动连接 WiFi 与 EMQX；掉线后由监听任务自动重连

> 注意：工程基于 STM32F103C8（中容量），FreeRTOS 与 HAL 库均已纳入仓库，Keil 编译产物（`.o/.crf/.axf` 等）已通过 `.gitignore` 排除。

## 目录结构

```
.
└── 基于RTOS的多模态智能环境监测与控制系统——智能家居物联网终端
    ├── Core
    │   ├── Inc/            各模块头文件
    │   └── Src/            业务源码（传感器、OLED、电机、AT 指令、任务调度等）
    ├── Drivers/            STM32 HAL 库与 CMSIS
    ├── FreeRTOS/           FreeRTOS 内核源码
    └── MDK-ARM/            Keil 工程（Key.uvprojx）
```

> 语音模块的天问 Block 工程（`.hd`、离线识别模型、播报 mp3）为本地开发文件，未纳入本仓库。

## 说明

本项目为个人学习实践项目，用于展示 STM32 + FreeRTOS + 物联网的完整链路实现。
