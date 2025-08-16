#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define KEY0_VALUE	0XF0
#define INVAKEY		0X00

int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
	unsigned char keyvalue;

	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开key驱动 */
	fd = open(filename, O_RDWR);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	while(1) {
		read(fd, &keyvalue, sizeof(keyvalue));
		if (keyvalue == KEY0_VALUE) printf("KEY0 is pressed, Value = %#X\r\n", keyvalue);
	}

	retvalue = close(fd); /* 关闭文件 */
	if(retvalue < 0){
		printf("file %s close failed!\r\n", argv[1]);
		return -1;
	}
	return 0;
}
