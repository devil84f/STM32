#ifndef __MPU6050_DRIVER_H
#define __MPU6050_DRIVER_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>  // 根据实际芯片系列修改

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
} MPU6050_Data;

/* 函数声明 */
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c);
void MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_Data *data);
void MPU6050_Convert_To_Real(MPU6050_Data *raw, float *accel, float *gyro);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */

