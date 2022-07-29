#include "kl_input.h"


int flag_key = 0;
pthread_rwlock_t rwlock;            // 定义读写锁
struct input_event in_ev_key = {0};


void key_input(void)
{
    int fd_key = -1;                            // 按键文件符

    pthread_rwlock_init(&rwlock, NULL);         // 对读写锁进行初始化
    fd_key = open("/dev/input/event2", O_RDONLY);
    while(1)
    {
        pthread_rwlock_wrlock(&rwlock);         // 以写模式获取锁 
        if (sizeof(struct input_event) != read(fd_key, &in_ev_key, sizeof(struct input_event)))
        {
            perror("read key_input error");
            exit(-1);
        }
        pthread_rwlock_unlock(&rwlock);         // 解锁 

        if (EV_KEY == in_ev_key.type)           // 按键事件
        {
            switch (in_ev_key.value)
            {
                case 0:
                    flag_key = 1;
                    printf("code<%d>,value<%d>: 松开\n", in_ev_key.code,in_ev_key.value);
                    break;
                case 1:
                    printf("code<%d>,value<%d>: 按下\n", in_ev_key.code,in_ev_key.value);
                    break;
                case 2:
                    printf("code<%d>,value<%d>: 长按\n", in_ev_key.code,in_ev_key.value);
                    break;
            }
        }
    }
    pthread_rwlock_destroy(&rwlock);    // 销毁锁
    close(fd_key);
}


void lcd_input(void)
{
    int fd_lcd = -1;    // lcd文件符
    int valid = 0;      // 记录lcd输入数据是否有效（信息更新表示有效，1有效，0无效）

    int x = 0, y = 0;   // lcd触摸点x和y坐标
    int down = -1;      // 记录BTN_TOUCH事件的value，1按下，0松开，-1移动

    struct input_event in_ev_lcd;

    fd_lcd = open("/dev/input/event1", O_RDONLY);
    while(1)
    {
        if (sizeof(struct input_event) != read(fd_lcd, &in_ev_lcd, sizeof(struct input_event)))
        {
            perror("read lcd_input error");
            exit(-1);
        }

        switch (in_ev_lcd.type)
        {
            case EV_KEY:                // lcd按键事件
                if (BTN_TOUCH == in_ev_lcd.code)
                {
                    down = in_ev_lcd.value;
                    valid = 1;
                }
                break;

            case EV_ABS:                // 绝对位移事件
                switch (in_ev_lcd.code)
                {
                    case ABS_X:         // X坐标
                        x = in_ev_lcd.value;
                        valid = 1;
                        break;
                    case ABS_Y:         // Y坐标
                        y = in_ev_lcd.value;
                        valid = 1;
                        break;
                }
                break;

            case EV_SYN:                // 同步事件
                if (SYN_REPORT == in_ev_lcd.code) 
                {
                    if (valid)          // 判断是否有效
                    {
                        switch (down)   // 判断状态
                        {
                            case 1:
                                printf("按下(%d, %d)\n", x, y);
                                break;
                            case 0:
                                printf("松开\n");
                                break;
                            case -1:
                                printf("移动(%d, %d)\n", x, y);
                                break;
                        }

                        valid = 0; // 重置valid
                        down = -1; // 重置down
                    }
                }
                break;
        }
    }
    close(fd_lcd);
}


