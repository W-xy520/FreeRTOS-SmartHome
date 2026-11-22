#include "Publish.h"
#include "ATCommand.h"
#include "string.h"


//AT+MQTTPUB=0,"对应主题","信息",0,0

//给对应主题发布信息所用的函数
uint8_t  Publish_Inf(char *name,char *inf)
{
    char information[100] = "AT+MQTTPUB=0,\"";
    strcat(information,name);
    strcat(information,"\",\"");
    strcat(information,inf);
    strcat(information,"\",0,0");
    AT_SendCommand(information);

    return 0;
}
