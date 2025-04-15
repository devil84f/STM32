#include "stm32f1xx_hal.h"
#include "MPU6050_driver.h"
#include <math.h>

#define RtA (57.2957795f)  // 弧度转角度，180/π ≈ 57.2957795
#define Ki  0.005f         // 积分系数
#define DT  0.005f          // 计算周期的一半，单位s
 
// 加速度和角速度
int16_t AX, AY, AZ, GX, GY, GZ;
 
typedef struct {
    float c1, c2, c3, a1, a2, a3, a;
} Degree;
 
Degree d;
 
typedef struct {
    float q0, q1, q2, q3;
    float exInt, eyInt, ezInt;
} Quater;
 
Quater q = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

/**
  * @brief  MPU6050初始化
  * @param  hi2c: I2C句柄指针
  * @retval 0-成功 1-失败
  */
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
  uint8_t check;
  
  // 检查设备ID
  HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, WHO_AM_I, 1, &check, 1, 100);
  if(check != 0x68) return 1;
  
  // 唤醒设备
  uint8_t data = 0x00;
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, PWR_MGMT_1, 1, &data, 1, 100);
  
  // 设置陀螺仪量程 ±250°/s
  data = 0x00;
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, GYRO_CONFIG, 1, &data, 1, 100);
  
  // 设置加速度计量程 ±2g
  data = 0x00;
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, ACCEL_CONFIG, 1, &data, 1, 100);
  
  // 配置数字低通滤波器(可选)
  data = 0x03;  // 加速度94Hz, 陀螺仪98Hz
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, 0x1A, 1, &data, 1, 100);
  
  return 0;
}

/**
  * @brief  读取所有传感器数据(原始值)
  * @param  hi2c: I2C句柄指针
  * @param  data: 数据存储结构体指针
  */
void MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_Data *data)
{
    uint8_t Rec_Data[14];
    
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, ACCEL_XOUT_H, 1, Rec_Data, 14, 100);
    
    data->Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data->Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    data->Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
    data->Gyro_X_RAW  = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
    data->Gyro_Y_RAW  = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
    data->Gyro_Z_RAW  = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);
}

/**
  * @brief  原始数据转换为实际物理量
  * @param  raw: 原始数据指针
  * @param  accel: 加速度数组(g) [x,y,z]
  * @param  gyro: 陀螺仪数组(°/s) [x,y,z]
  */
void MPU6050_Convert_To_Real(MPU6050_Data *raw, float *accel, float *gyro)
{
    // 加速度计转换 (±2g范围)
    accel[0] = raw->Accel_X_RAW / 16384.0f;
    accel[1] = raw->Accel_Y_RAW / 16384.0f;
    accel[2] = raw->Accel_Z_RAW / 16384.0f;
    
    // 陀螺仪转换 (±250°/s范围)
    gyro[0] = raw->Gyro_X_RAW / 131.0f;
    gyro[1] = raw->Gyro_Y_RAW / 131.0f;
    gyro[2] = raw->Gyro_Z_RAW / 131.0f;
	
	// 转换为rad/s
	gyro[0] /= RtA;
	gyro[1] /= RtA;
	gyro[2] /= RtA;
	
	// 加速度归一化
    float norm = sqrtf(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);
    if (norm > 1e-6f) {  // 防止除零
        float inv_norm = 1.0f / norm;
        accel[0] *= inv_norm;
        accel[1] *= inv_norm;
        accel[2] *= inv_norm;
    }
}
