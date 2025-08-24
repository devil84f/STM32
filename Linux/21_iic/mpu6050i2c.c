#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    short resive_data[6];  //保存收到的 mpu6050转换结果数据，依次为 AX(x轴角度), AY, AZ 。GX(x轴加速度), GY ,GZ
 
    /*打开文件*/
    int fd = open("/dev/mpu6050", O_RDWR);
    if(fd < 0)
    {
		printf("open file : %s failed !\n", argv[0]);
        perror("why error:");
		return -1;
	}
    /*读取数据*/
    int error = read(fd,resive_data,12);
    if(error < 0)
    {
        printf("write file error! \n");
        close(fd);
        /*判断是否关闭成功*/
    }
 
    /*打印数据*/
    printf("AX=%d, AY=%d, AZ=%d ",(int)resive_data[0],(int)resive_data[1],(int)resive_data[2]);
	printf("    GX=%d, GY=%d, GZ=%d \n \n",(int)resive_data[3],(int)resive_data[4],(int)resive_data[5]);
 
 
    /*关闭文件*/
    error = close(fd);
    if(error < 0)
    {
        printf("close file error! \n");
    }
    
    return 0;
}