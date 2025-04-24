#ifndef __MPU6050_DRIVER_H
#define __MPU6050_DRIVER_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>  // 根据实际芯片系列修改
#include "i2c.h"

/* 寄存器定义 */
#define MPU6050_ADDR        0x68 << 1  // 7-bit地址左移1位
#define SMPLRT_DIV          0x19
#define GYRO_CONFIG         0x1B
#define ACCEL_CONFIG        0x1C
#define ACCEL_XOUT_H        0x3B
#define TEMP_OUT_H          0x41
#define GYRO_XOUT_H         0x43
#define PWR_MGMT_1          0x6B
#define WHO_AM_I            0x75

/* 数据结构体 */
typedef struct {
    int16_t  Accel_X_RAW;
    int16_t  Accel_Y_RAW;
    int16_t  Accel_Z_RAW;
    int16_t  Gyro_X_RAW;
    int16_t  Gyro_Y_RAW;
    int16_t Gyro_Z_RAW;
	
	// 滤波后的数据（新增）
    float Accel_X_Filtered;
    float Accel_Y_Filtered;
    float Accel_Z_Filtered;
    float Gyro_X_Filtered;
    float Gyro_Y_Filtered;
    float Gyro_Z_Filtered;
} MPU6050_Data;

// 卡尔曼滤波器结构体（放在头文件或全局定义）
typedef struct {
    float Q;    // 过程噪声
    float R;    // 测量噪声
    float P;    // 估计误差协方差
    float K;    // 卡尔曼增益
    float Out;  // 滤波输出
} KalmanFilter;

// 互补滤波结构体
typedef struct {
    float factor;  // 滤波系数 (0.0~1.0)
    float prev;    // 上一次滤波值
} ComplFilter;

// 全局滤波变量（6轴：3轴加速度 + 3轴陀螺仪）
static KalmanFilter EKF_Accel[3];  // 加速度计卡尔曼滤波
static ComplFilter CF_Gyro[3];     // 陀螺仪互补滤波

#define PI 3.1415926535f
#define RtA 57.2957795f  // 弧度转角度
#define AtR 0.0174532925f  // 角度转弧度
 
typedef struct 
{
    float q0, q1, q2, q3;
} Quaternion;

/* 函数声明 */
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c);
void MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_Data *data);
void MPU6050_Convert_To_Real(MPU6050_Data *raw, float *accel, float *gyro);
float ComplFilter_Update(ComplFilter *cf, float measurement);
void KalmanFilter_Update(KalmanFilter *kf, float measurement);

void MPU6050_ReadSensors(float *ax, float *ay, float *az, float *gx, float *gy, float *gz);
void NormalizeAccel(float *ax, float *ay, float *az);
void ComputeError(float ax, float ay, float az, float *error_x, float *error_y, float *error_z);
void UpdateQuaternion(float gx, float gy, float gz, float error_x, float error_y, float error_z, float dt);
void ComputeEulerAngles(float *pitch, float *roll, float *yaw);
void GetAngles(float *pitch, float *roll, float *yaw, float dt);
	
#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */

