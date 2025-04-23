#include "stm32f1xx_hal.h"
#include "MPU6050_driver.h"
#include <math.h>

#define RtA (57.2957795f)  // ����ת�Ƕȣ�180/�� �� 57.2957795
 
// ��Ԫ���ṹ��
typedef struct {
    float q0, q1, q2, q3;
} Quaternion;

// ģ���ڲ�״̬
static Quaternion quat = {1.0f, 0.0f, 0.0f, 0.0f}; // ��ʼ��Ԫ��������ת��
static float beta = 0.1f;                          // Madgwick�˲�����
static float dt = 0.01f;                           // Ĭ�ϲ���ʱ�䣨100Hz��

/**
  * @brief  MPU6050��ʼ��
  * @param  hi2c: I2C���ָ��
  * @retval 0-�ɹ� 1-ʧ��
  */
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
  uint8_t check;
  
  // ����豸ID
  HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, WHO_AM_I, 1, &check, 1, 100);
  if(check != 0x68) return 1;
  
  // �����豸
  uint8_t data = 0x00;
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, PWR_MGMT_1, 1, &data, 1, 100);
  
  // �������������� ��250��/s
  data = 0x00;
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, GYRO_CONFIG, 1, &data, 1, 100);
  
  // ���ü��ٶȼ����� ��2g
  data = 0x00;
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, ACCEL_CONFIG, 1, &data, 1, 100);
  
  // �������ֵ�ͨ�˲���(��ѡ)
  data = 0x03;  // ���ٶ�94Hz, ������98Hz
  HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, 0x1A, 1, &data, 1, 100);
  
  return 0;
}

/**
  * @brief  ��ȡ���д���������(ԭʼֵ)
  * @param  hi2c: I2C���ָ��
  * @param  data: ���ݴ洢�ṹ��ָ��
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
  * @brief  ԭʼ����ת��Ϊʵ��������
  * @param  raw: ԭʼ����ָ��
  * @param  accel: ���ٶ�����(g) [x,y,z]
  * @param  gyro: ����������(��/s) [x,y,z]
  */
void MPU6050_Convert_To_Real(MPU6050_Data *raw, float *accel, float *gyro)
{
    // ���ٶȼ�ת�� (��2g��Χ)
    accel[0] = raw->Accel_X_RAW / 16384.0f;
    accel[1] = raw->Accel_Y_RAW / 16384.0f;
    accel[2] = raw->Accel_Z_RAW / 16384.0f;
    
    // ������ת�� (��250��/s��Χ)
    gyro[0] = raw->Gyro_X_RAW / 131.0f;
    gyro[1] = raw->Gyro_Y_RAW / 131.0f;
    gyro[2] = raw->Gyro_Z_RAW / 131.0f;
	
	// ת��Ϊrad/s
	gyro[0] /= RtA;
	gyro[1] /= RtA;
	gyro[2] /= RtA;
	
	// ���ٶȹ�һ��
    float norm = sqrtf(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);
    if (norm > 1e-6f) {  // ��ֹ����
        float inv_norm = 1.0f / norm;
        accel[0] *= inv_norm;
        accel[1] *= inv_norm;
        accel[2] *= inv_norm;
    }
}

// ��ʼ����̬����ģ��
void Attitude_Init(float sample_rate, float beta_gain) {
    beta = beta_gain;
    dt = 1.0f / sample_rate;
    quat.q0 = 1.0f; // ������Ԫ��
    quat.q1 = quat.q2 = quat.q3 = 0.0f;
}

// Madgwick�˲�������Ԫ��
static void MadgwickUpdate(float accel[3], float gyro[3]) {
    float ax = accel[0], ay = accel[1], az = accel[2];
    float gx = gyro[0], gy = gyro[1], gz = gyro[2];

    // 1. ��һ�����ٶȼ�����
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm == 0.0f) return;
    ax /= norm; ay /= norm; az /= norm;

    // 2. ����Ԥ����������
    float vx = 2.0f * (quat.q1 * quat.q3 - quat.q0 * quat.q2);
    float vy = 2.0f * (quat.q0 * quat.q1 + quat.q2 * quat.q3);
    float vz = quat.q0 * quat.q0 - quat.q1 * quat.q1 - quat.q2 * quat.q2 + quat.q3 * quat.q3;

    // 3. ���������ٶȼ� vs Ԥ�⣩
    float ex = (ay * vz - az * vy);
    float ey = (az * vx - ax * vz);
    float ez = (ax * vy - ay * vx);

    // 4. ��������������
    gx += beta * ex;
    gy += beta * ey;
    gz += beta * ez;

    // 5. ��Ԫ������
    quat.q0 += (-quat.q1 * gx - quat.q2 * gy - quat.q3 * gz) * 0.5f * dt;
    quat.q1 += ( quat.q0 * gx + quat.q2 * gz - quat.q3 * gy) * 0.5f * dt;
    quat.q2 += ( quat.q0 * gy - quat.q1 * gz + quat.q3 * gx) * 0.5f * dt;
    quat.q3 += ( quat.q0 * gz + quat.q1 * gy - quat.q2 * gx) * 0.5f * dt;

    // 6. ��һ����Ԫ��
    norm = sqrtf(quat.q0 * quat.q0 + quat.q1 * quat.q1 + quat.q2 * quat.q2 + quat.q3 * quat.q3);
    if (norm == 0.0f) return;
    quat.q0 /= norm; quat.q1 /= norm; quat.q2 /= norm; quat.q3 /= norm;
}

// ������̬
void Attitude_Update(float accel[3], float gyro[3]) {
    MadgwickUpdate(accel, gyro);
}

// ��ȡŷ���ǣ���λ�����ȣ�
EulerAngles Attitude_GetEulerAngles(void) {
    EulerAngles angles;
    
    // Roll (X-axis rotation)
    angles.roll = atan2f(2.0f * (quat.q0 * quat.q1 + quat.q2 * quat.q3),
                         1.0f - 2.0f * (quat.q1 * quat.q1 + quat.q2 * quat.q2));
    
    // Pitch (Y-axis rotation)
    float sinp = 2.0f * (quat.q0 * quat.q2 - quat.q3 * quat.q1);
    if (fabsf(sinp) >= 1.0f) {
        angles.pitch = copysignf(M_PI / 2.0f, sinp); // ��������ֵ
    } else {
        angles.pitch = asinf(sinp);
    }
    
    // Yaw (Z-axis rotation)
    angles.yaw = atan2f(2.0f * (quat.q0 * quat.q3 + quat.q1 * quat.q2),
                        1.0f - 2.0f * (quat.q2 * quat.q2 + quat.q3 * quat.q3));
    
    return angles;
}

