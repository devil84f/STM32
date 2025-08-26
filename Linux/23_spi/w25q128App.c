#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#define W25Q128_GET_ID _IOR('W', 0, char[3])

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    unsigned char id_buf[3] = {0};
    
    // 打开设备
    fd = open("/dev/w25q128", O_RDWR);
    if (fd < 0) {
        printf("Failed to open device: %s\n", strerror(errno));
        return -1;
    }
    
    printf("Device opened successfully\n");
    
    // 方法1: 使用read读取ID
    printf("Reading ID via read()...\n");
    ret = read(fd, id_buf, sizeof(id_buf));
    if (ret < 0) {
        printf("Failed to read ID: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    printf("ID (via read): 0x%02X 0x%02X 0x%02X\n", 
           id_buf[0], id_buf[1], id_buf[2]);
    
    // 方法2: 使用ioctl读取ID
    printf("Reading ID via ioctl()...\n");
    memset(id_buf, 0, sizeof(id_buf));
    ret = ioctl(fd, W25Q128_GET_ID, id_buf);
    if (ret < 0) {
        printf("Failed to get ID via ioctl: %s\n", strerror(errno));
    } else {
        printf("ID (via ioctl): 0x%02X 0x%02X 0x%02X\n", 
               id_buf[0], id_buf[1], id_buf[2]);
    }
    
    close(fd);
    printf("Test completed\n");
    
    return 0;
}