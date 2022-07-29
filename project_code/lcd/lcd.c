#include "lcd.h"
#include "font.h"


int width;                           //LCD宽度
int height;                          //LCD高度
unsigned short *screen_base = NULL;  //LCD显存基地址
int fb_fd = -1;                      //LCD设备文件描述符


int fb_dev_init(void)                // LCD初始化
{
    struct fb_var_screeninfo fb_var = {0};
    struct fb_fix_screeninfo fb_fix = {0};
    unsigned long screen_size;


    fb_fd = open(FB_DEV, O_RDWR);   // 打开framebuffer设备
    if (0 > fb_fd) 
    {
        fprintf(stderr, "open error: %s: %s\n", FB_DEV, strerror(errno));
        return -1;
    }

    ioctl(fb_fd, FBIOGET_VSCREENINFO, &fb_var);     // 获取framebuffer设备信息
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_fix);


    screen_size = fb_fix.line_length * fb_var.yres; // 屏幕尺寸信息？？
    width = fb_var.xres;
    height = fb_var.yres;


    screen_base = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);    // 内存映射
    if (MAP_FAILED == (void *)screen_base) 
    {
        perror("mmap error");
        close(fb_fd);
        return -1;
    }

    memset(screen_base, 0x80, screen_size);     // LCD背景设置
    return 0;
}


void lcd_drawpoint(int x, int y, unsigned short color)  // 指定区域画点
{
    *(screen_base + width * y + x) = color;
}


void lcd_showchar(int x, int y, unsigned char num, unsigned short color)    // 指定区域显示字符或数字
{
    int t, t1;
    int y0 = y;
    unsigned char temp;

	for(t = 0; t < 36; t++)             // 字符对应字节数
	{   
		temp = asc2_2412[num][t];       // 调用字体
		for(t1 = 0; t1 < 8; t1++)
		{
			if(temp & 0x80) lcd_drawpoint(x, y, color);    // 如果bit为1则显示
            else            lcd_drawpoint(x, y, 0x0000);   // 显示为背景

			temp <<= 1;
			y++;

			if((y - y0) == 24)          // 字符显示宽度
			{
				y = y0;
				x++;                    // 从下一行开始
			}
		}	 
	}	    	   	 	  
}


void lcd_show_string(int x, int y, unsigned char *p, unsigned short color)  // 指定区域显示字符串
{
    unsigned char num;

    while((*p >= 'A') && (*p <= 'Z'))   // 可显示字符区间
    {
        num = *p - 55;                  // 字母对应数组索引
        lcd_showchar(x, y, num, color);

        p += 1;
        x += 25;
    }  
}




