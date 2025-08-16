#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

#define LEDOFF 	0
#define LEDON 	1
#define CLOSE_CMD		_IO(0XEF, 1)
#define OPEN_CMD		_IO(0XEF, 2)
#define SETPERIOD_CMD	_IOW(0XEF, 3, int)
/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
	unsigned int cmd;
	unsigned long arg;
	unsigned char str[100];

	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开led驱动 */
	fd = open(filename, O_RDWR);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	while (1) {
		printf("Input CMD:");
		retvalue = scanf("%d", &cmd);
		if (retvalue != 1){
			gets(str);
		}
		if(cmd == 1) cmd = CLOSE_CMD;
		else if (cmd == 2) cmd = OPEN_CMD;
		else if (cmd == 3){
			cmd = SETPERIOD_CMD;
			printf("Input Timer Period:");
			retvalue = scanf("%d", &arg);
			if (retvalue != 1){
				gets(str);
			}
		}
		ioctl(fd, cmd, arg);
	}
	close(fd);
	return 0;
}
