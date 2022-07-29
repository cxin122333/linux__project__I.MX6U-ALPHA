# 定义变量
# source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi 
cross_compile	=	${CC}					# 需要使能环境变量，否则可能默认是gcc，并非交叉编译
target			=	proj_code_exe


# 模式规则，获取c文件（带目录），建立o文件
src1	=	$(wildcard user/*.c)
src2	=	$(wildcard v4l2/*.c)
src3	=	$(wildcard lcd/*.c)
src4	=	$(wildcard bmp/*.c)
src5	=	$(wildcard media/*.c)
src6	=	$(wildcard input/*.c)

obj1	=	$(patsubst %.c,%.o,$(src1))
obj2	=	$(patsubst %.c,%.o,$(src2))
obj3	=	$(patsubst %.c,%.o,$(src3))
obj4	=	$(patsubst %.c,%.o,$(src4))
obj5	=	$(patsubst %.c,%.o,$(src5))
obj6	=	$(patsubst %.c,%.o,$(src6))


# 链接生成目标文件，链接alsa文件需要指定链接库-lasound
# $^所有的依赖文件
$(target)	:	$(obj1)	$(obj2)	$(obj3)	$(obj4)	$(obj5)	$(obj6)
	$(cross_compile)	-o	$(target)	$^	-lasound	-lpthread


# 编译c文件，gcc默认去c文件所在目录查找h文件，其他目录需用-I指定
$(obj1)	:	$(src1)
	$(cross_compile)	-c	$(src1)	-I	v4l2/	-I	lcd/	-I	bmp/	-I	media/	-I	input/	-o	$(obj1)

$(obj2)	:	$(src2)
	$(cross_compile)	-c	$(src2)	-o	$(obj2)

$(obj3)	:	$(src3)
	$(cross_compile)	-c	$(src3)	-o	$(obj3)

$(obj4)	:	$(src4)
	$(cross_compile)	-c	$(src4)	-o	$(obj4)

$(obj5)	:	$(src5)
	$(cross_compile)	-c	$(src5)	-o	$(obj5)

$(obj6)	:	$(src6)
	$(cross_compile)	-c	$(src6)	-o	$(obj6)


# 声明伪目标
.PHONY:	clean

clean:
	rm	$(target)	$(obj1)	$(obj2)	$(obj3) $(obj4)	$(obj5)	$(obj6)




