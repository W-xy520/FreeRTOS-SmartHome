#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "Serial.h"



void Serial1_SendString(char *String)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)String, strlen(String), 1000);
}

void Serial1_Printf(char *format, ...)
{
	char String[120];
	va_list arg;
	va_start(arg,format);
	vsprintf(String,format,arg);
	va_end(arg);
	Serial1_SendString(String);
}

void Serial2_SendString(char *String)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)String, strlen(String), 1000);
}

void Serial2_Printf(char *format, ...)
{
	char String[120];
	va_list arg;
	va_start(arg,format);
	vsprintf(String,format,arg);
	va_end(arg);
	Serial2_SendString(String);
}

void Serial3_SendString(char *String)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)String, strlen(String), 1000);
}

void Serial3_Printf(char *format, ...)
{
	char String[120];
	va_list arg;
	va_start(arg,format);
	vsprintf(String,format,arg);
	va_end(arg);
	Serial3_SendString(String);
}
