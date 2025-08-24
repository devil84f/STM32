#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#define MPU6050_DEVICE "/dev/mpu6050"

/* 数据结构，与驱动中保持一致 */
struct mpu6050_data {
    int16_t AX; 
    int16_t AY; 
    int16_t AZ; 
    int16_t GX; 
    int16_t GY; 
    int16_t GZ;
};

/* 运行标志，用于优雅退出 */
static volatile int keep_running = 1;

/* 信号处理函数，用于Ctrl+C退出 */
void signal_handler(int sig)
{
    keep_running = 0;
    printf("\n正在退出...\n");
}

/* 打印使用说明 */
void print_usage(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help     显示帮助信息\n");
    printf("  -c, --count N  读取N次数据后退出\n");
    printf("  -i, --interval MS 设置读取间隔(毫秒，默认100ms)\n");
    printf("  -r, --raw      显示原始数据\n");
    printf("  -v, --verbose  显示详细信息\n");
    printf("  -s, --silent   静默模式，只显示数值\n");
    printf("\n示例:\n");
    printf("  %s               # 持续读取数据\n", program_name);
    printf("  %s -c 10         # 读取10次数据\n", program_name);
    printf("  %s -i 500        # 每500ms读取一次\n", program_name);
    printf("  %s -r -v         # 显示原始数据和详细信息\n", program_name);
}

/* 将原始数据转换为实际物理量 */
void convert_to_physical(const struct mpu6050_data *raw, float *accel, float *gyro)
{
    /* MPU6050灵敏度设置：
     * 加速度计：±16g -> 2048 LSB/g
     * 陀螺仪：±2000°/s -> 16.4 LSB/°/s
     */
    const float accel_scale = 1.0f / 2048.0f;   // ±16g
    const float gyro_scale = 1.0f / 16.4f;      // ±2000°/s
    
    accel[0] = raw->AX * accel_scale;  // X轴加速度 (g)
    accel[1] = raw->AY * accel_scale;  // Y轴加速度 (g)
    accel[2] = raw->AZ * accel_scale;  // Z轴加速度 (g)
    
    gyro[0] = raw->GX * gyro_scale;    // X轴角速度 (°/s)
    gyro[1] = raw->GY * gyro_scale;    // Y轴角速度 (°/s)
    gyro[2] = raw->GZ * gyro_scale;    // Z轴角速度 (°/s)
}

/* 显示数据 */
void display_data(const struct mpu6050_data *data, int show_raw, int verbose, int silent)
{
    float accel[3], gyro[3];
    
    if (silent) {
        printf("%d,%d,%d,%d,%d,%d\n", 
               data->AX, data->AY, data->AZ,
               data->GX, data->GY, data->GZ);
        return;
    }
    
    convert_to_physical(data, accel, gyro);
    
    if (show_raw) {
        printf("原始数据: ");
        printf("AX=%6d, AY=%6d, AZ=%6d, ", data->AX, data->AY, data->AZ);
        printf("GX=%6d, GY=%6d, GZ=%6d\n", data->GX, data->GY, data->GZ);
    }
    
    if (verbose || !show_raw) {
        printf("加速度: X=%7.3fg, Y=%7.3fg, Z=%7.3fg | ", accel[0], accel[1], accel[2]);
        printf("角速度: X=%7.2f°/s, Y=%7.2f°/s, Z=%7.2f°/s\n", gyro[0], gyro[1], gyro[2]);
    }
}

int main(int argc, char *argv[])
{
	int i;
    int fd;
    int ret;
    int count = -1;          // 读取次数，-1表示无限循环
    int interval = 100;      // 读取间隔(毫秒)
    int show_raw = 0;        // 是否显示原始数据
    int verbose = 0;         // 详细模式
    int silent = 0;          // 静默模式
    
    struct mpu6050_data data;
    
    // 解析命令行参数
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count") == 0) {
            if (i + 1 < argc) {
                count = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) {
            if (i + 1 < argc) {
                interval = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--raw") == 0) {
            show_raw = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            silent = 1;
        }
    }
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    
    // 打开设备
    fd = open(MPU6050_DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("无法打开MPU6050设备");
        printf("请确保:\n");
        printf("1. MPU6050驱动已加载\n");
        printf("2. 设备节点 %s 存在\n", MPU6050_DEVICE);
        printf("3. 有足够的权限访问设备\n");
        return -1;
    }
    
    printf("MPU6050测试程序启动\n");
    printf("设备: %s\n", MPU6050_DEVICE);
    printf("采样间隔: %dms\n", interval);
    if (count > 0) {
        printf("读取次数: %d\n", count);
    } else {
        printf("读取模式: 持续读取 (按Ctrl+C退出)\n");
    }
    printf("----------------------------------------\n");
    
    int read_count = 0;
    while (keep_running && (count < 0 || read_count < count)) {
        // 读取数据
        ret = read(fd, &data, sizeof(data));
        if (ret != sizeof(data)) {
            if (ret < 0) {
                perror("读取数据失败");
            } else {
                fprintf(stderr, "读取数据不完整: 期望 %zu 字节, 实际 %d 字节\n", 
                        sizeof(data), ret);
            }
            break;
        }
        
        read_count++;
        
        if (!silent) {
            printf("[%04d] ", read_count);
        }
        
        display_data(&data, show_raw, verbose, silent);
        
        // 如果不是最后一次读取，则等待
        if (keep_running && (count < 0 || read_count < count)) {
            usleep(interval * 1000);
        }
    }
    
    // 关闭设备
    close(fd);
    
    if (!silent) {
        printf("----------------------------------------\n");
        printf("测试完成，总共读取 %d 次数据\n", read_count);
    }
    
    return 0;
}