#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

int main() {
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;

    // 1. 打开帧缓冲设备
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    // 2. 获取固定屏幕信息
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        exit(2);
    }

    // 3. 获取可变屏幕信息
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        exit(3);
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    printf("Line length: %d bytes\n", finfo.line_length);

    // 4. 计算屏幕缓冲区的大小
    screensize = vinfo.yres_virtual * finfo.line_length;
    // screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8; // 另一种计算方式

    // 5. 将帧缓冲区映射到用户空间
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((long)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");

    // --- 在这里进行绘图操作 ---
    int x, y;
    long int location = 0;

    // 示例：绘制一个渐变彩条
    for (y = 0; y < vinfo.yres; y++) {
        for (x = 0; x < vinfo.xres; x++) {
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                       (y + vinfo.yoffset) * finfo.line_length;

            // 根据你的颜色深度格式编写像素
            // 假设你的屏幕格式是 16bpp RGB565
            if (vinfo.bits_per_pixel == 16) {
                // 计算一个渐变颜色
                unsigned short blue =  (x * 31 / vinfo.xres) & 0x1F;
                unsigned short green = ((y * 63 / vinfo.yres) & 0x3F) << 5;
                unsigned short red =   ((x * 31 / vinfo.xres) & 0x1F) << 11;
                unsigned short color = red | green | blue;

                *((unsigned short*)(fbp + location)) = color;
            } 
            // 如果是 32bpp ARGB8888（注意字节序，实际可能是ABGR或BGRA，需根据驱动确认）
            else if (vinfo.bits_per_pixel == 32) {
                // 这是一个示例，格式可能需要调整
                int blue =  (x * 255 / vinfo.xres);
                int green = (y * 255 / vinfo.yres);
                int red =   (x * 255 / vinfo.xres);
                int alpha = 0; // 不透明

                // 常见的格式是 0xAARRGGBB，但具体要看驱动
                *((unsigned int*)(fbp + location)) = (alpha << 24) | (red << 16) | (green << 8) | blue;
            } else {
                printf("Unsupported bpp: %d\n", vinfo.bits_per_pixel);
                break;
            }
        }
    }
    // --- 绘图结束 ---

    // 6. 解除映射并关闭设备
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}