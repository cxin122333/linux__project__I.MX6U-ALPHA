#include "v4l2.h"


int v4l2_fd = -1;                       //摄像头设备文件描述符
int frm_width, frm_height;              //视频帧宽度和高度


cam_fmt cam_fmts[10];
cam_buf_info buf_infos[FRAMEBUFFER_COUNT];


int v4l2_dev_init(const char *device)   // 摄像头初始化
{
    struct v4l2_capability cap = {0};

    v4l2_fd = open(device, O_RDWR);     // 打开设备节点文件，保存文件描述符；使用O_RDWR指定读和写权限
    if (0 > v4l2_fd) 
    {
        fprintf(stderr, "open error: %s: %s\n", device, strerror(errno));
        return -1;
    }

    ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap);  // VIDIOC_QUERYCAP指令，查询设备功能和属性

    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities))   // V4L2_CAP_VIDEO_CAPTURE，表示支持视频采集；此处判断是否为视频采集设备
    {
        fprintf(stderr, "Error: %s: No capture video device!\n", device);
        close(v4l2_fd);
        return -1;
    }

    return 0;
}


void v4l2_enum_formats(void)            // 枚举所有像素格式
{
    struct v4l2_fmtdesc fmtdesc = {0};  // 描述像素格式相关信息

    fmtdesc.index = 0;  // 枚举编号，初始为0,每调用一次ioctl后加1,直到调用失败
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;     // 将type设为V4L2_BUF_TYPE_VIDEO_CAPTURE，表示视频采集的像素格式

    while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc))  // VIDIOC_ENUM_FMT指令，枚举设备支持的所有像素格式
    {
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;  // 枚举出格式与描述信息，存放在数组中
        strcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description);
        fmtdesc.index++;
    }
}


void v4l2_print_formats(void)           // 打印摄像头支持的分辨率及帧率
{
    struct v4l2_frmsizeenum frmsize = {0};
    struct v4l2_frmivalenum frmival = {0};

    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 设置type与pixel_format，确定设备的哪种功能、哪种像素格式支持的视频帧大小
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    int i;

    for (i = 0; cam_fmts[i].pixelformat; i++) 
    {
        printf("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat, cam_fmts[i].description);

        frmsize.index = 0;      // 枚举编号，初始为0，枚举摄像头支持的所有视频采集分辨率
        frmsize.pixel_format = cam_fmts[i].pixelformat;     // pixel_format，指定像素格式
        frmival.pixel_format = cam_fmts[i].pixelformat;

        while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize))   // VIDIOC_ENUM_FRAMESIZES，枚举设备支持的所有视频采集分辨率
        {
            printf("size<%d*%d> ", frmsize.discrete.width, frmsize.discrete.height);   // 打印视频帧大小信息（包括视频帧的宽、高度），即视频采集分辨率大小
            frmsize.index++;

            frmival.index = 0;  // 枚举摄像头视频采集帧率
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;

            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival))   // VIDIOC_ENUM_FRAMEINTERVALS，枚举设备支持的所有帧率
            {
                printf("<%dfps>", frmival.discrete.denominator / frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");
        }
        printf("\n");
    }
}


int v4l2_set_format(void)               // 设置摄像头格式（重点）
{
    struct v4l2_format fmt = {0};
    struct v4l2_streamparm streamparm = {0};

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // type设为V4L2_BUF_TYPE_VIDEO_CAPTURE，表示视频采集的像素格式

    fmt.fmt.pix.width = 480;    // LCD宽度（1024）
    fmt.fmt.pix.height = 272;   // LCD高度（600）
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;  // RGB565像素格式


    if (0 > ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt))     // VIDIOC_S_FMT指令，设置设备的格式，ioctl使用fmt所指对象的数据去设置
    {
        fprintf(stderr, "ioctl error: VIDIOC_S_FMT: %s\n", strerror(errno));
        return -1;
    }
    if (V4L2_PIX_FMT_RGB565 != fmt.fmt.pix.pixelformat)     // 判断是否设为RGB565格式，没有则表示设备不支持
    {
        fprintf(stderr, "Error: the device does not support RGB565 format!\n");
        return -1;
    }

    frm_width = fmt.fmt.pix.width;      //实际获取的帧宽度
    frm_height = fmt.fmt.pix.height;    //实际获取的帧高度
    printf("视频帧大小<%d * %d>\n", frm_width, frm_height);


    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm);     // VIDIOC_G_PARM指令，获取设备的流类型相关参数

    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability)     // V4L2_CAP_TIMEPERFRAME，表示设备支持timeperframe字段，应用层可设置设备的视频采集帧率
    {
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30;          //30fps
        if (0 > ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm))             // VIDIOC_S_PARM指令，设置设备的流类型相关参数
        {
            fprintf(stderr, "ioctl error: VIDIOC_S_PARM: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}


int v4l2_init_buffer(void)              // 申请帧缓冲、内存映射、入队
{
    struct v4l2_requestbuffers reqbuf = {0};
    struct v4l2_buffer buf = {0};

    reqbuf.count = FRAMEBUFFER_COUNT;           // 申请帧缓冲的数量
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    
    if (0 > ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf))    // VIDIOC_REQBUFS指令，申请帧缓冲，用于存储一帧图像数据，ioctl会根据reqbuf所指对象的信息进行申请
    {
        fprintf(stderr, "ioctl error: VIDIOC_REQBUFS: %s\n", strerror(errno));
        return -1;
    }


    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;     // 内存映射前，设置type
    buf.memory = V4L2_MEMORY_MMAP;

    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) // 帧缓冲区由内核维护，应用程序不能直接读取数据，需要将其映射到用户空间
    {
        ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf);      // 映射前需查询帧缓冲信息，如帧缓冲长度、偏移量等；ioctl将获取数据写入buf指针所指对象

        // 申请帧缓冲后，调用mmap将帧缓冲映射到用户空间，各帧缓冲对应映射区的起始地址保存在buf_infos数组，后面可通过映射区读取摄像头数据
        buf_infos[buf.index].length = buf.length;
        buf_infos[buf.index].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, v4l2_fd, buf.m.offset);

        if (MAP_FAILED == buf_infos[buf.index].start) 
        {
            perror("mmap error");
            return -1;
        }
    }


    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++)     // 入队
    {
        if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &buf))  // VIDIOC_QBUF指令，将帧缓冲放入内核的帧缓冲队列中
        {
            fprintf(stderr, "ioctl error: VIDIOC_QBUF: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}


int v4l2_stream_on(void)                 // 摄像头开启采集
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  // type设为V4L2_BUF_TYPE_VIDEO_CAPTURE，表示视频采集的像素格式

    if (0 > ioctl(v4l2_fd, VIDIOC_STREAMON, &type))     // VIDIOC_STREAMON指令，开启视频采集
    {
        fprintf(stderr, "ioctl error: VIDIOC_STREAMON: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}





