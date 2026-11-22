#ifndef __ATCOMMAND_H
#define __ATCOMMAND_H

#include "FreeRTOS_Demo.h"
#include "stdint.h"
#include "FreeRTOS.h"

typedef struct{
    uint16_t read;
    uint16_t write;
    uint8_t Buffer[256];
}Sbuf;

extern Sbuf AT_Buf;

uint8_t Buffer_Read(uint8_t *r,Sbuf *Buffer);
void Buffer_Write(uint8_t *w,Sbuf *Buffer);
uint8_t AT_SendCommand(char *string);
void ATCommand_Init(void);

#endif
