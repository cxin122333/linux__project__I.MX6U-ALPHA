#ifndef _MEDIA_H
#define _MEDIA_H



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// #include <signal.h>
#include <alsa/asoundlib.h>



#define PCM_PLAYBACK_DEV    "hw:0,0"        // 逻辑设备名 ——> 声卡0上的PCM设备0；播放情况下对应设备文件名 ——> /dev/snd/pcmC0D0p


// WAV音频文件解析相关数据结构申明
typedef struct WAV_RIFF 
{
    char ChunkID[4];                /* "RIFF" */
    u_int32_t ChunkSize;         	/* 从下一个地址开始到文件末尾的总字节数 */
    char Format[4];              	/* "WAVE" */
} __attribute__ ((packed)) RIFF_t;



typedef struct WAV_FMT 
{
    char Subchunk1ID[4];         	/* "fmt " */
    u_int32_t Subchunk1Size;      	/* 16 for PCM */
    u_int16_t AudioFormat;       	/* PCM = 1*/
    u_int16_t NumChannels;       	/* Mono = 1, Stereo = 2, etc. */
    u_int32_t SampleRate;        	/* 8000, 44100, etc. */
    u_int32_t ByteRate;           	/* = SampleRate * NumChannels * BitsPerSample/8 */
    u_int16_t BlockAlign;         	/* = NumChannels * BitsPerSample/8 */
    u_int16_t BitsPerSample;     	/* 8bits, 16bits, etc. */
} __attribute__ ((packed)) FMT_t;

extern FMT_t wav_fmt;



typedef struct WAV_DATA 
{
    char Subchunk2ID[4];         	/* "data" */
    u_int32_t Subchunk2Size;     	/* data size */
} __attribute__ ((packed)) DATA_t;



extern void *buf;                       // 指向应用程序缓冲区的指针

extern int fd;                          // 指向WAV音频文件的文件描述符
extern unsigned int buf_bytes;          // 应用程序缓冲区的大小（字节为单位）
extern unsigned int periods;            // 周期数（设备驱动层buffer的大小）
extern snd_pcm_t *pcm;                  // pcm句柄
extern snd_pcm_uframes_t period_size;   // 周期大小（单位: 帧）



int snd_pcm_init(void);
int open_wav_file(const char *file);
int media_play_init(void);                     // 播放初始化
int voice__play(void);



#endif




