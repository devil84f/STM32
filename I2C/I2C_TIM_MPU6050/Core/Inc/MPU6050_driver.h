#ifndef __MPU6050_DRIVER_H
#define __MPU6050_DRIVER_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>  // ����ʵ��оƬϵ���޸�
#include "i2c.h"

/* �Ĵ������� */
#define MPU6050_ADDR        0x68 << 1  // 7-bit��ַ����1λ
#define SMPLRT_DIV          0x19
#define GYRO_CONFIG         0x1B
#define ACCEL_CONFIG        0x1C
#define ACCEL_XOUT_H        0x3B
#define TEMP_OUT_H          0x41
#define GYRO_XOUT_H         0x43
#define PWR_MGMT_1          0x6B
#define WHO_AM_I            0x75

/* ���ݽṹ�� */
typedef struct {
    int16_t  Accel_X_RAW;
    int16_t  Accel_Y_RAW;
    int16_t  Accel_Z_RAW;
    int16_t  Gyro_X_RAW;
    int16_t  Gyro_Y_RAW;
    int16_t Gyro_Z_RAW;
	
	// �˲�������ݣ�������
    float Accel_X_Filtered;
    float Accel_Y_Filtered;
    float Accel_Z_Filtered;
    float Gyro_X_Filtered;
    float Gyro_Y_Filtered;
    float Gyro_Z_Filtered;
} MPU6050_Data;

// �������˲����ṹ�壨����ͷ�ļ���ȫ�ֶ��壩
typedef struct {
    float Q;    // ��������
    float R;    // ��������
    float P;    // �������Э����
    float K;    // ����������
    float Out;  // �˲����
} KalmanFilter;

// �����˲��ṹ��
typedef struct {
    float factor;  // �˲�ϵ�� (0.0~1.0)
    float prev;    // ��һ���˲�ֵ
} ComplFilter;

// ȫ���˲�������6�᣺3����ٶ� + 3�������ǣ�
static KalmanFilter EKF_Accel[3];  // ���ٶȼƿ������˲�
static ComplFilter CF_Gyro[3];     // �����ǻ����˲�

#define PI 3.1415926535f
#define RtA 57.2957795f  // ����ת�Ƕ�
#define AtR 0.0174532925f  // �Ƕ�ת����
 
typedef struct 
{
    float q0, q1, q2, q3;
} Quaternion;

/* �������� */
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

