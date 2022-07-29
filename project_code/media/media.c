#include "media.h"



// static定义和声明是同时的，所以不要放在头文件中，其他文件不可用
FMT_t wav_fmt;

void *buf = NULL;                  	    // 指向应用程序缓冲区的指针

int fd = -1;                         	// 指向WAV音频文件的文件描述符
unsigned int buf_bytes;                	// 应用程序缓冲区的大小（字节为单位）
unsigned int periods = 16;            	// 周期数（设备驱动层buffer的大小）
snd_pcm_t *pcm = NULL;                  // pcm句柄
snd_pcm_uframes_t period_size = 1024; 	// 周期大小（单位: 帧）



int snd_pcm_init(void)
{
    int ret;
    snd_pcm_hw_params_t *hwparams = NULL;

    /* 打开PCM设备 */
    ret = snd_pcm_open(&pcm, PCM_PLAYBACK_DEV, SND_PCM_STREAM_PLAYBACK, 0);
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_open error: %s: %s\n", PCM_PLAYBACK_DEV, snd_strerror(ret));
        return -1;
    }

    /* 实例化hwparams对象 */
    snd_pcm_hw_params_malloc(&hwparams);

    /* 获取PCM设备当前硬件配置,对hwparams进行初始化 */
    ret = snd_pcm_hw_params_any(pcm, hwparams);
    if (0 > ret)
    {
        fprintf(stderr, "snd_pcm_hw_params_any error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /************************************ 设置参数 *************************************/
    /* 设置访问类型: 交错模式 */
    ret = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_hw_params_set_access error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /* 设置数据格式: 有符号16位、小端模式 */
    ret = snd_pcm_hw_params_set_format(pcm, hwparams, SND_PCM_FORMAT_S16_LE);
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_hw_params_set_format error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /* 设置采样率 */
    ret = snd_pcm_hw_params_set_rate(pcm, hwparams, wav_fmt.SampleRate, 0);
    if (0 > ret)
    {
        fprintf(stderr, "snd_pcm_hw_params_set_rate error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /* 设置声道数: 双声道 */
    ret = snd_pcm_hw_params_set_channels(pcm, hwparams, wav_fmt.NumChannels);
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_hw_params_set_channels error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /* 设置周期大小: period_size */
    ret = snd_pcm_hw_params_set_period_size(pcm, hwparams, period_size, 0);
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_hw_params_set_period_size error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /* 设置周期数（驱动层环形缓冲区buffer的大小）: periods */
    ret = snd_pcm_hw_params_set_periods(pcm, hwparams, periods, 0);
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_hw_params_set_periods error: %s\n", snd_strerror(ret));
        goto err2;
    }

    /* 使配置生效 */
    ret = snd_pcm_hw_params(pcm, hwparams);
    snd_pcm_hw_params_free(hwparams);   // 释放hwparams对象占用的内存
    if (0 > ret) 
    {
        fprintf(stderr, "snd_pcm_hw_params error: %s\n", snd_strerror(ret));
        goto err1;
    }

    buf_bytes = period_size * wav_fmt.BlockAlign; // 变量赋值，一个周期的字节大小
    return 0;

err2:
    snd_pcm_hw_params_free(hwparams);   // 释放内存
err1:
    snd_pcm_close(pcm);                 // 关闭pcm设备
    return -1;
}


int open_wav_file(const char *file)     // 解析wav文件，对头部数据进行校验，获取音频格式信息和数据的位置偏移量
{
    int ret;
    RIFF_t wav_riff;
    DATA_t wav_data;

    fd = open(file, O_RDONLY);
    if (0 > fd) 
    {
        fprintf(stderr, "open error: %s: %s\n", file, strerror(errno));
        return -1;
    }

    /* 读取RIFF chunk */
    ret = read(fd, &wav_riff, sizeof(RIFF_t));
    if (sizeof(RIFF_t) != ret) 
    {
        if (0 > ret) perror("read error");
        else         fprintf(stderr, "check error: %s\n", file);
        
        close(fd);
        return -1;
    }

    if (strncmp("RIFF", wav_riff.ChunkID, 4) || strncmp("WAVE", wav_riff.Format, 4))    // 校验
    {
        fprintf(stderr, "check error: %s\n", file);
        close(fd);
        return -1;
    }

    /* 读取sub-chunk-fmt */
    ret = read(fd, &wav_fmt, sizeof(FMT_t));
    if (sizeof(FMT_t) != ret) 
    {
        if (0 > ret) perror("read error");
        else         fprintf(stderr, "check error: %s\n", file);

        close(fd);
        return -1;
    }

    if (strncmp("fmt ", wav_fmt.Subchunk1ID, 4))        // 校验
    {
        fprintf(stderr, "check error: %s\n", file);
        close(fd);
        return -1;
    }

    /* 打印音频文件的信息 */
    printf("<<音频文件格式信息>>\n\n");
    printf(" file name:     %s\n", file);
    printf(" Subchunk1Size: %u\n", wav_fmt.Subchunk1Size);
    printf(" AudioFormat:   %u\n", wav_fmt.AudioFormat);
    printf(" NumChannels:   %u\n", wav_fmt.NumChannels);
    printf(" SampleRate:    %u\n", wav_fmt.SampleRate);
    printf(" ByteRate:      %u\n", wav_fmt.ByteRate);
    printf(" BlockAlign:    %u\n", wav_fmt.BlockAlign);
    printf(" BitsPerSample: %u\n\n", wav_fmt.BitsPerSample);

    /* sub-chunk-data */
    if (0 > lseek(fd, sizeof(RIFF_t) + 8 + wav_fmt.Subchunk1Size, SEEK_SET)) 
    {
        perror("lseek error");
        close(fd);
        return -1;
    }

    while(sizeof(DATA_t) == read(fd, &wav_data, sizeof(DATA_t))) 
    {
        /* 找到sub-chunk-data */
        if (!strncmp("data", wav_data.Subchunk2ID, 4)) return 0;    // 校验

        if (0 > lseek(fd, wav_data.Subchunk2Size, SEEK_CUR)) 
        {
            perror("lseek error");
            close(fd);
            return -1;
        }
    }

    fprintf(stderr, "check error: %s\n", file);
    return -1;
}


int media_play_init(void)
{
    int ret;

    if (open_wav_file("/home/root/monitor_voice.wav")) exit(EXIT_FAILURE);  // 打开WAV音频文件
    if (snd_pcm_init()) goto err1;      // 初始化PCM Playback设备

    buf = malloc(buf_bytes);            // 申请读缓冲区
    if (NULL == buf) 
    {
        perror("malloc error");
        goto err2;
    }

    return 0;

err2:
    snd_pcm_close(pcm); // 关闭pcm设备
err1:
    close(fd);          // 关闭音频文件
    exit(EXIT_FAILURE);
}


int voice__play(void)
{
    int ret;
    snd_pcm_sframes_t avail;
    

    avail = snd_pcm_avail_update(pcm);  // 获取当前可写入帧数
    while (avail >= period_size)        // 一次循环，写入一周期
    {
        memset(buf, 0x00, buf_bytes);   //buf清零


        ret = read(fd, buf, buf_bytes); // 读取wav文件
        if (0 >= ret)
        {
            snd_pcm_drop(pcm);          // 停止运行
            lseek(fd, 0, SEEK_SET);     // 返回文件头
            read(fd, buf, buf_bytes);
            snd_pcm_prepare(pcm);       // 设备重新准备
        }

        ret = snd_pcm_writei(pcm, buf, period_size);    // 向环形缓冲区写入数据进行播放，写入一周期
        if (0 > ret)                 goto err3;
        else if (ret < period_size)  lseek(fd, (ret-period_size) * wav_fmt.BlockAlign, SEEK_CUR);


        avail = snd_pcm_avail_update(pcm);  //再次获取、更新avail
    }

    return 0;

err3:
    free(buf);          // 释放内存
    snd_pcm_close(pcm); // 关闭pcm设备
    close(fd);          // 关闭音频文件
    exit(EXIT_FAILURE);
}




