#ifndef _BMP_H
#define _BMP_H


#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define BI_BITFIELDS 0x3

typedef char    BYTE;   // 单字节
typedef short   WORD;   // 双字节
typedef int     DWORD;  // 四字节
typedef int     LONG;   // 四字节



int Rgb565ConvertBmp(char *buf,int width,int height, const char *filename);



#endif





