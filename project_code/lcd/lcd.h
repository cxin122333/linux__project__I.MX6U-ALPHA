#ifndef _LCD_H
#define _LCD_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/fb.h>


#define FB_DEV       "/dev/fb0"         //LCD设备节点

extern int width;                       //LCD宽度
extern int height;                      //LCD高度
extern unsigned short *screen_base;     //LCD显存基地址
extern int fb_fd;                       //LCD设备文件描述符


int fb_dev_init(void);                  // LCD初始化
void lcd_drawpoint(int x, int y, unsigned short color);   // 指定区域画点
void lcd_showchar(int x, int y, unsigned char num, unsigned short color);   // 指定区域显示字符或数字
void lcd_show_string(int x, int y, unsigned char *p, unsigned short color); // 指定区域显示字符串



#endif




