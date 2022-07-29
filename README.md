# linux__project__I.MX6U-ALPHA
Linux独立开发项目，基于正点原子I.MX6ULL-ALPHA开发板，出厂自带Linux系统

除开发板外，其他所需硬件：LCD屏、ov5640摄像头

注意：
1、交叉编译前，需使能环境变量：source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi
2、编译后下载至开发板，输入命令运行：./可执行文件名 /dev/video1
3、开发板中wav文件存放位置：/home/root/monitor_voice.wav，可在程序中更改
