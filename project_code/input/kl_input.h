#ifndef _KL_INPUT_H
#define _KL_INPUT_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <pthread.h> 


extern int flag_key;
extern pthread_rwlock_t rwlock;         // 声明读写锁
extern struct input_event in_ev_key;


void key_input(void);
void lcd_input(void);



#endif



