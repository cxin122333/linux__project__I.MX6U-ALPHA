/***************************************************************
项目日志：
（0529）更改程序编写风格，整理程序内容；
（0529）LCD白色背景更改：fb_dev_init——>memset——>0xFF
（0529）调整ov5640视频帧大小：v4l2_set_format——>fmt.fmt.pix.width = 480、fmt.fmt.pix.height = 272

（0530）拆解函数，使功能模块化、松耦合；
（0530）添加功能———>灰度处理、移动检测、图像二值化；

（0531）加入图像滤波（九宫格滤除稀疏单点噪声）；
（0531）纠正程序变量格式，防止数据保存时类型被转换、内容被更改；

（0601）删除最佳迭代法、九宫格滤波。在三帧差分法中设定阈值，利用该阈值同时完成二值化与滤波
（0601）取消LCD右下角二值化图像显示，将左下角目标信息用二值化显示；右下区域显示字符信息；

（0602）main文件中分离v4l2、LCD模块程序，建立模块子文件，将项目程序模块化；
（0602）编写makefile文件，用make命令编译项目文件；

（0603）完善makefile文件
（0603）添加功能———>LCD字符与数字显示，屏幕右下区域显示信息；

（0604）添加功能———>视频帧保存为BMP图片，在nfs目录下共享

（0605）ubuntu安装向日葵，设置开机自启动、无密码。实现终端远程查看
（0605）ubuntu安装双网卡，用于连接开发板与上网

（0608）移植音频模块ALSA
（0608）更改交叉编译器：arm-linux-gnueabihf-gcc ———> arm-poky-linux-gnueabihf-gcc（编译前使能环境变量）

（0610）更改makefile文件

（0630）添加输入设备————>按键、lcd单点触控
（0630）增加多线程————>main线程、key线程、lcd线程、安防模式线程、生活模式线程（模式线程只运行一个）

（0701）多线程中添加读写锁，操作按键值

（0702）增加模式切换功能
 ***************************************************************/



#include "lcd.h"
#include "v4l2.h"
#include "bmp.h"
#include "media.h"
#include "kl_input.h"
#include <pthread.h> 


int flag_switch = 1;            // 模式切换标志位（初始为安防模式）

int count = 0;                  // 统计目标像素数量
int flag_bit = 0;               // 标志位，触发截屏存储
char screen_shot[480*272*2];    // 存储截屏数据，一个数组元素1字节，一个rgb565像素需要两个元素存储

unsigned short origin_image[816][480];      // 三帧原始图像数据，由于RGB565像素为两字节，所以保存格式为unsigned short
unsigned short gray_image[816][480];        // 三帧灰度图像数据，灰度像素只有一个字节大小，但一个LCD像素显示2字节数据
unsigned short three_frame_differ[272][480];// 三帧差分法处理，得到一帧目标图像，再进行二值化处理


/*******************************  安防功能函数  *********************************/
void copy_image_data(void)                      // 拷贝三帧图像数据
{
    int i, cnt = 0;
    unsigned short *start;                      // 64位linux系统，指针为8字节，指针指向内容为unsigned short类型，即像素为无符号短整型
    struct v4l2_buffer buf = {0};               // 帧缓冲
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;     // V4L2_BUF_TYPE_VIDEO_CAPTURE，表示视频采集的像素格式
    buf.memory = V4L2_MEMORY_MMAP;

    for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++)
    {
        ioctl(v4l2_fd, VIDIOC_DQBUF, &buf);     // VIDIOC_DQBUF指令，帧缓冲出队

        start = buf_infos[buf.index].start;     // 帧缓冲对应映射区的起始地址，视频帧数据存放于此
        for (i = 0; i < frm_height; i++)        // 视频帧行数
        {
            // 拷贝一行像素数据，memcpy的复制单位是1字节，而1个RGB像素占2字节，占一个数组元素
            memcpy(*origin_image + cnt, start, frm_width * 2);      // *origin_image是二维数组首元素地址
            cnt += frm_width;                   // 指向二维数组下一行
            start += frm_width;                 // 下一行像素地址
        }

        ioctl(v4l2_fd, VIDIOC_QBUF, &buf);      // 再入队
    }
}


void image_grey_processing(void)                // 图像灰度处理
{
    int i, cnt = 0;
    unsigned short rgb565_data, gray_data;
    unsigned short color_R, color_G, color_B;

    for (i = 0; i < 816*480; i++)
    {
        rgb565_data = *(*origin_image + cnt);
        color_R = rgb565_data >> 11;            // 0000,0000,000x,xxxx
        color_G = (rgb565_data >> 5) & 0x3f;    // 0000,0xxx,xxxx,xxxx & 0011,1111 ——> 右端对齐，左边补零
        color_B = rgb565_data & 0x1f;           // xxxx,xxxx,xxxx,xxxx & 0001,1111

        gray_data = color_R*0.299 + color_G*0.587 + color_B*0.114;   // 灰度公式
        *(*gray_image + cnt) = gray_data;
        cnt += 1;
    }
}


void frame_differ(void)                         // 三帧差分法检测移动信息
{
    int i, num = 0;
    int cnt1 = 0, cnt2 = 272*480, cnt3 = 272*480*2;
    unsigned short sec_to_fir, thr_to_sec;

    for (i = 0; i < 272*480; i++)
    {
        sec_to_fir = abs(*(*gray_image + cnt2) - *(*gray_image + cnt1));     // 前两张图灰度值做差
        thr_to_sec = abs(*(*gray_image + cnt3) - *(*gray_image + cnt2));     // 后两张图灰度值做差

        if (sec_to_fir == thr_to_sec) *(*three_frame_differ + cnt1) = sec_to_fir;
        else                          *(*three_frame_differ + cnt1) = 0;

        cnt1++; cnt2++; cnt3++;

        // 使用简单二值化，将目标像素值改为255；并设定阈值消除噪点，效果良好
        if(*(*three_frame_differ + i) > 3)
        {
            *(*three_frame_differ + i) = 255;
            num++;        // 统计目标像素数量，噪声白点可忽略
        }
        else
            *(*three_frame_differ + i) = 0;
    }

    count = num;
}
void Binarization(void) // 二值化灰度图像（废弃）
{
    // int i, j, k;
    // int sum1, cnt1, sum2, cnt2;
	// unsigned short threshold1, threshold2, max=0, min=0; //阈值1、阈值2、最大灰度值、最小灰度值

	// for(i = 0; i < 272*480; i++)                // 找出图像最大、最小灰度值
	// {
	// 	if(*(*three_frame_differ + i) > max)    max = *(*three_frame_differ + i);
	// 	if(*(*three_frame_differ + i) < min)    min = *(*three_frame_differ + i);      
	// } 

	// threshold1 = (max + min) * 0.5;		        // 计算初始阈值

    // do  
    // {
    //     threshold2 = threshold1;	            //继续循环，迭代未完成，记录当前阈值
    // 	sum1=0; cnt1=0;
	// 	sum2=0; cnt2=0;

    //     for (j = 0; j < 272*480; j++)
    //     {
	// 		if(*(*three_frame_differ + j) > threshold1)
	// 		{
	// 			sum1 += *(*three_frame_differ + j);     // 统计大于阈值的像素点个数，及灰度值之和
	// 			cnt1 += 1;		
	// 		}
	// 		else
	// 		{
	// 			sum2 += *(*three_frame_differ + j);	    // 小于阈值的像素点个数，及灰度值之和
	// 			cnt2 += 1;	
	// 		}
    //     }
    //     threshold1 = (sum1/cnt1 + sum2/cnt2) * 0.5;     //更新阈值

    // } while (threshold1!=threshold2);

	// for(k = 0; k < 272*480; k++)                // 迭代完成，二值化图像
	// {
	// 	if(*(*three_frame_differ + k) > threshold1)  *(*binary_imagine + k) = 255;
	// 	else    *(*binary_imagine + k) = 0; 
	// }
}
void image_filter(void) // 滤除图像噪声（废弃）
{
    // int i, j;
    // unsigned short a[9];

    // for (i = 1; i <= 270; i+=3)
    // {
    //     for (j = 1; j <= 478; j+=3)
    //     {
    //         a[0] = *(*binary_imagine + 480 * (i - 1) + (j - 1));
    //         a[1] = *(*binary_imagine + 480 * (i - 1) +    j   );
    //         a[2] = *(*binary_imagine + 480 * (i - 1) + (j + 1));
    //         a[3] = *(*binary_imagine + 480 *    i    + (j - 1));
    //         a[4] = *(*binary_imagine + 480 *    i    +    j   );
    //         a[5] = *(*binary_imagine + 480 *    i    + (j + 1));
    //         a[6] = *(*binary_imagine + 480 * (i + 1) + (j - 1));
    //         a[7] = *(*binary_imagine + 480 * (i + 1) +    j   );
    //         a[8] = *(*binary_imagine + 480 * (i + 1) + (j + 1));

    //         if (a[0]+a[1]+a[2]+a[3]+a[4]+a[5]+a[6]+a[7]+a[8] == 255)    // 适用于单个噪点散列分布
    //         {
    //             *(*binary_imagine + 480 * (i - 1) + (j - 1)) = 0;
    //             *(*binary_imagine + 480 * (i - 1) +    j   ) = 0;
    //             *(*binary_imagine + 480 * (i - 1) + (j + 1)) = 0;
    //             *(*binary_imagine + 480 *    i    + (j - 1)) = 0;
    //             *(*binary_imagine + 480 *    i    +    j   ) = 0;
    //             *(*binary_imagine + 480 *    i    + (j + 1)) = 0;
    //             *(*binary_imagine + 480 * (i + 1) + (j - 1)) = 0;
    //             *(*binary_imagine + 480 * (i + 1) +    j   ) = 0;
    //             *(*binary_imagine + 480 * (i + 1) + (j + 1)) = 0;
    //         }
    //     }
    // }
}
void LCD_display(void)                          // LCD显示
{
    int i, j, k;
    int cnt = 0, cnt1 = 0;
    unsigned short *base;                       // 显存基地址（指针指向内容为unsigned short类型，即LCD像素类型，可显示两字节）

    for (i = 0; i < FRAMEBUFFER_COUNT; i++)     // 三帧原始图像，每帧需重置LCD显存地址
    {
        base = screen_base;                     // 从LCD左上角开始，显存大小 = 1024 × 600
        for (j = 0; j < frm_height; j++)
        {
            memcpy(base, *origin_image + cnt, frm_width * 2);   //  显示原始图像
            base += 512;                                        // 从屏幕另一半开始
            memcpy(base, *gray_image + cnt, frm_width * 2);     //  显示灰度图像
            base += 512;                        // LCD从下一行开始
            cnt += frm_width;                   // 指向二维数组下一行
        }


        cnt1 = 0;       // 大循环运行3次，但目标灰度和二值化图像只有一帧，因此要重置数组偏移量
        base = screen_base + 1024*300;          // 从LCD下半部分开始
        for (k = 0; k < frm_height; k++)
        {
            memcpy(base, *three_frame_differ + cnt1, frm_width * 2);  // 显示移动目标
            base += 1024;                       // LCD从下一行开始
            cnt1 += frm_width;                  // 指向二维数组下一行
        }
    }
}


void warning_process(void)                      // 截屏存储、语音警告
{
    int i, cnt1 = 0, cnt2 = 0;
    unsigned char a[13] = "OBJECTMOVING";
    int indiv, ten, hundred, thousand, myriad;  // 个十百千万

    indiv    = count / 1 % 10;
    ten      = count / 10 % 10;
    hundred  = count / 100 % 10;
    thousand = count / 1000 % 10;
    myriad   = count / 10000 % 10;

    lcd_showchar(718, 410, myriad, 0xf800);     // 目标像素数量
    lcd_showchar(733, 410, thousand, 0xf800);
    lcd_showchar(748, 410, hundred, 0xf800);
    lcd_showchar(763, 410, ten, 0xf800);
    lcd_showchar(778, 410, indiv, 0xf800);

    if (count > 20)                             // 图像噪点数 < 2，设置移动像素阈值
    {
        lcd_show_string(600, 480, a, 0xf800);
        lcd_showchar(900, 480, 36, 0xf800);     // 感叹号字符

        flag_bit ++;
        if (flag_bit == 3)                      // 触发截屏保存，只保存一次
        {
            for (i = 0; i < 272; i++)
            {
                // 先获取一帧像素数据，注意origin_image数组元素两字节，screen_shot数组元素一字节
                memcpy(screen_shot + cnt1, *origin_image + cnt2, frm_width * 2);
                cnt1 += frm_width * 2;
                cnt2 += frm_width;
            }
            Rgb565ConvertBmp(screen_shot, 480, 272, "/cxin_nfs/test.bmp");
        }

        // if (SND_PCM_STATE_PAUSED == snd_pcm_state(pcm))
        // snd_pcm_pause(pcm, 0);      // 恢复
    }
    else
    {
        lcd_show_string(600, 480, a, 0x0000);   // 显示黑色背景
        lcd_showchar(900, 480, 36, 0x0000);     // 感叹号
    
        // if (SND_PCM_STATE_RUNNING == snd_pcm_state(pcm))
        // {
        //     snd_pcm_pause(pcm, 1);      // 暂停
        //     // lseek(fd, 0, SEEK_SET);     // 返回文件头
        // }
    }
}


/*******************************  多线程  *********************************/
void *new_thread_key(void *arg)                 // key线程执行函数
{ 
    printf("key输入线程: 进程ID<%d>  线程ID<%lu>\n", getpid(), pthread_self());
    key_input();
    return (void *)0; 
}


void *new_thread_lcd(void *arg)                 // lcd线程执行函数
{ 
    printf("lcd触控线程: 进程ID<%d>  线程ID<%lu>\n", getpid(), pthread_self());
    lcd_input();
    return (void *)0; 
}


void pthread_input(void)                        // 输入设备线程
{
    int ret_key, ret_lcd;
    pthread_t tid_key, tid_lcd; 

    // 第一个null表示线程属性默认，第二个null表示无需给线程执行函数传参
    // 线程创建成功返回0；失败返回错误号，且tid_key内容不确定
    ret_key = pthread_create(&tid_key, NULL, new_thread_key, NULL); 
    if (ret_key) 
    { 
        fprintf(stderr, "key_pthread_create_Error: %s\n", strerror(ret_key)); 
        exit(-1); 
    }

    ret_lcd = pthread_create(&tid_lcd, NULL, new_thread_lcd, NULL); 
    if (ret_lcd) 
    { 
        fprintf(stderr, "lcd_pthread_create_Error: %s\n", strerror(ret_lcd)); 
        exit(-1); 
    }
}


void *security_modle(void *arg)                 // 线程执行函数（安防模式）
{
    int ret;
    ret = pthread_detach(pthread_self());       // 线程自行分离，退出后自动回收线程资源
    if (ret) 
    { 
        fprintf(stderr, "security_modle_detach error: %s\n", strerror(ret)); 
        exit(-1); 
    }

    printf("安防模式线程: 进程ID<%d>  线程ID<%lu>\n", getpid(), pthread_self());
    memset(screen_base, 0x80, 1024*600*2);      // LCD背景设置
    unsigned char a[6] = "MODEL";               // 显示字符
    unsigned char b[9] = "SECURITY";
    lcd_show_string(550, 320, a, 0xf800);       // 0xf800——>红色
    lcd_show_string(700, 320, b, 0xf800);

    while(1)
    {
        copy_image_data();                      // 拷贝三帧图像数据
        image_grey_processing();                // 图像灰度处理
        frame_differ();                         // 三帧差分法检测移动信息
        LCD_display();                          // LCD显示
        warning_process();                      // 截屏存储、语音警告

        // voice__play();

        if ((flag_key == 1)&&(0 != in_ev_key.value))
        {
            flag_key = 0;                       // 按键标志位置0,等按键松开后置1
            printf("安防线程end\n");
            flag_switch = 1;                    // 模式运行结束，切换标志位置1
            pthread_exit(NULL); 
        }
    }
    return (void *)0; 
}


void *life_modle(void *arg)                     // 线程执行函数（生活模式）
{
    int ret;
    ret = pthread_detach(pthread_self());       // 线程自行分离，退出后自动回收线程资源
    if (ret) 
    { 
        fprintf(stderr, "life_modle_detach error: %s\n", strerror(ret)); 
        exit(-1); 
    }

    printf("生活模式线程: 进程ID<%d>  线程ID<%lu>\n", getpid(), pthread_self());
    memset(screen_base, 0x00, 1024*600*2);      // LCD背景设置
    unsigned char a[6] = "MODEL";               // 显示字符
    unsigned char b[5] = "LIFE";
    lcd_show_string(550, 320, a, 0xf800);       // 0xf800——>红色
    lcd_show_string(700, 320, b, 0xf800);

    while(1)
    {
        if ((flag_key == 1)&&(0 != in_ev_key.value))
        {
            flag_key = 0;                       // 按键标志位置0,等按键松开后置1
            printf("生活线程end\n");
            flag_switch = 2;                    // 模式运行结束，切换标志位置2
            pthread_exit(NULL); 
        }
    }
    return (void *)0; 
}


/*******************************  运行方式：./proj_code_exe /dev/video1  *********************************/
// 交叉编译前，使能环境变量：source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

int main(int argc, char *argv[])                      // argv[0] ——> ./文件名； argv[1] ——> /dev/video1
{
    if (2 != argc)               exit(EXIT_FAILURE);  // 检查输入参数
    if (fb_dev_init())           exit(EXIT_FAILURE);  // 初始化LCD，失败则退出
    /********************************************/
    if (v4l2_dev_init(argv[1]))  exit(EXIT_FAILURE);  // 初始化摄像头
    v4l2_enum_formats();                              // 枚举所有像素格式
    v4l2_print_formats();                             // 打印摄像头支持的分辨率及帧率
    if (v4l2_set_format())       exit(EXIT_FAILURE);  // 设置摄像头格式（重点）
    if (v4l2_init_buffer())      exit(EXIT_FAILURE);  // 申请帧缓冲、内存映射、入队
    if (v4l2_stream_on())        exit(EXIT_FAILURE);  // 摄像头开启采集
    /********************************************/
    media_play_init();                                // 音频设备初始化
    /********************************************/
    pthread_input();                                  // 启动输入设备线程
    /********************************************/
    printf("main主线程: 进程ID<%d>  线程ID<%lu>\n", getpid(), pthread_self());
    /********************************************/
    int ret_security, ret_life;
    pthread_t tid_security, tid_life; 
    /********************************************/
    while (1)
    {
        if (flag_switch == 1)
        {
            flag_switch = 0;
            ret_life = pthread_create(&tid_life, NULL, life_modle, NULL); 
            if (ret_life)
            { 
                fprintf(stderr, "life_pthread_create_Error: %s\n", strerror(ret_life)); 
                exit(-1); 
            }
        }
        else if (flag_switch == 2)
        {
            flag_switch = 0;
            ret_security = pthread_create(&tid_security, NULL, security_modle, NULL);
            if (ret_security)
            {
                fprintf(stderr, "security_pthread_create_Error: %s\n", strerror(ret_security)); 
                exit(-1); 
            }
        }
    }

    exit(EXIT_SUCCESS);
}






