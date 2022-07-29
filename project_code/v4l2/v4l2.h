#ifndef _V4L2_H
#define _V4L2_H


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
#include <linux/videodev2.h>            // V4L2指令定义在该文件
#include <linux/fb.h>


#define FRAMEBUFFER_COUNT   3           //帧缓冲数量

extern int v4l2_fd;                     //摄像头设备文件描述符
extern int frm_width, frm_height;       //视频帧宽度和高度


typedef struct camera_format            // 摄像头像素格式及描述信息
{
    unsigned char description[32];      //字符串描述信息
    unsigned int pixelformat;           //像素格式
} cam_fmt;
extern cam_fmt cam_fmts[10];


typedef struct cam_buf_info             // 描述一个帧缓冲
{
    unsigned short *start;              //帧缓冲起始地址
    unsigned long length;               //帧缓冲长度
} cam_buf_info;
extern cam_buf_info buf_infos[FRAMEBUFFER_COUNT];



int v4l2_dev_init(const char *device);  // 摄像头初始化
void v4l2_enum_formats(void);           // 枚举所有像素格式
void v4l2_print_formats(void);          // 打印摄像头支持的分辨率及帧率
int v4l2_set_format(void);              // 设置摄像头格式
int v4l2_init_buffer(void);             // 申请帧缓冲、内存映射、入队
int v4l2_stream_on(void);               // 摄像头开启采集



#endif







