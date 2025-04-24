#include "stm32f1xx_hal.h"
#include "MPU6050_driver.h"
#include <math.h>
#include <stdio.h>

Quaternion q = {1.0f, 0.0f, 0.0f, 0.0f};  // ��ʼ����Ԫ��

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
    // ��ʼ�����ٶȼƿ������˲�
    for (int i = 0; i < 3; i++) {
        EKF_Accel[i].Q = 0.02f;   // ����������Ӱ����Ӧ�ٶȣ�
        EKF_Accel[i].R = 0.001f;  // ����������Ӱ��Բ���ֵ�����ζȣ�
        EKF_Accel[i].P = 0.0f;
        EKF_Accel[i].K = 0.0f;
        EKF_Accel[i].Out = 0.0f;
    }

    // ��ʼ�������ǻ����˲�
    for (int i = 0; i < 3; i++) {
        CF_Gyro[i].factor = 0.15f;  // �˲�ϵ����0.15 = ������ռ15%��
        CF_Gyro[i].prev = 0.0f;     // ��ʼ��ʷֵ
    }
	
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, ACCEL_XOUT_H, 1, Rec_Data, 14, 100);
    
    data->Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data->Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    data->Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
    data->Gyro_X_RAW  = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
    data->Gyro_Y_RAW  = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
    data->Gyro_Z_RAW  = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);
	
	// ���ٶȼƿ������˲���X/Y/Z��
    KalmanFilter_Update(&EKF_Accel[0], (float)data->Accel_X_RAW);
    KalmanFilter_Update(&EKF_Accel[1], (float)data->Accel_Y_RAW);
    KalmanFilter_Update(&EKF_Accel[2], (float)data->Accel_Z_RAW);

    // �����ǻ����˲���X/Y/Z��
    data->Gyro_X_Filtered = ComplFilter_Update(&CF_Gyro[0], (float)data->Gyro_X_RAW);
    data->Gyro_Y_Filtered = ComplFilter_Update(&CF_Gyro[1], (float)data->Gyro_Y_RAW);
    data->Gyro_Z_Filtered = ComplFilter_Update(&CF_Gyro[2], (float)data->Gyro_Z_RAW);

    // ��ѡ�������˲���ļ��ٶ�ֵ�������Ҫ��
    data->Accel_X_Filtered = EKF_Accel[0].Out;
    data->Accel_Y_Filtered = EKF_Accel[1].Out;
    data->Accel_Z_Filtered = EKF_Accel[2].Out;
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
    accel[0] = raw->Accel_X_Filtered / 16384.0f;
    accel[1] = raw->Accel_Y_Filtered / 16384.0f;
    accel[2] = raw->Accel_Z_Filtered / 16384.0f;
    
    // ������ת�� (��250��/s��Χ)
    gyro[0] = raw->Gyro_X_Filtered / 131.0f;
    gyro[1] = raw->Gyro_Y_Filtered / 131.0f;
    gyro[2] = raw->Gyro_Z_Filtered / 131.0f;
	
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

void KalmanFilter_Update(KalmanFilter *kf, float measurement) {
    // Ԥ��
    kf->P += kf->Q;

    // ����
    kf->K = kf->P / (kf->P + kf->R);
    kf->Out += kf->K * (measurement - kf->Out);
    kf->P *= (1 - kf->K);
}

float ComplFilter_Update(ComplFilter *cf, float measurement) {
    cf->prev = cf->prev * (1.0f - cf->factor) + measurement * cf->factor;
    return cf->prev;
}

void MPU6050_ReadSensors(float *ax, float *ay, float *az, float *gx, float *gy, float *gz)
{
	MPU6050_Data mpu_data;
	MPU6050_Read_All(&hi2c2, &mpu_data);

    *ax = mpu_data.Accel_X_Filtered / 16384.0f;;  // ��2g
    *ay = mpu_data.Accel_Y_Filtered / 16384.0f;
    *az = mpu_data.Accel_Z_Filtered / 16384.0f;
    *gx = mpu_data.Gyro_X_Filtered / 131.0f * AtR;  // ��250��/s ת����
    *gy = mpu_data.Gyro_Y_Filtered / 131.0f * AtR;
    *gz = mpu_data.Gyro_Z_Filtered / 131.0f * AtR;
}

void NormalizeAccel(float *ax, float *ay, float *az) 
{
    float norm = sqrt(*ax * *ax + *ay * *ay + *az * *az);
    *ax /= norm;
    *ay /= norm;
    *az /= norm;
}

void ComputeError(float ax, float ay, float az, float *error_x, float *error_y, float *error_z) 
{
    float gravity_x = 2 * (q.q1 * q.q3 - q.q0 * q.q2);
    float gravity_y = 2 * (q.q0 * q.q1 + q.q2 * q.q3);
    float gravity_z = 1 - 2 * (q.q1 * q.q1 + q.q2 * q.q2);
 
    *error_x = (ay * gravity_z - az * gravity_y);
    *error_y = (az * gravity_x - ax * gravity_z);
    *error_z = (ax * gravity_y - ay * gravity_x);
}

void UpdateQuaternion(float gx, float gy, float gz, float error_x, float error_y, float error_z, float dt) 
{
    float Kp = 2.0f;  // �����˲�ϵ��
 
    float q0_dot = -q.q1 * gx - q.q2 * gy - q.q3 * gz;
    float q1_dot = q.q0 * gx - q.q3 * gy + q.q2 * gz;
    float q2_dot = q.q3 * gx + q.q0 * gy - q.q1 * gz;
    float q3_dot = -q.q2 * gx + q.q1 * gy + q.q0 * gz;
 
    q.q0 += (q0_dot + Kp * error_x) * dt;
    q.q1 += (q1_dot + Kp * error_y) * dt;
    q.q2 += (q2_dot + Kp * error_z) * dt;
    q.q3 += (q3_dot) * dt;
 
    // ��һ����Ԫ����������ֵ����Խ��Խ��
    float norm = sqrt(q.q0 * q.q0 + q.q1 * q.q1 + q.q2 * q.q2 + q.q3 * q.q3);
    q.q0 /= norm;
    q.q1 /= norm;
    q.q2 /= norm;
    q.q3 /= norm;
}

void ComputeEulerAngles(float *pitch, float *roll, float *yaw) 
{
    *roll = atan2(2 * (q.q2 * q.q3 + q.q0 * q.q1), q.q0 * q.q0 - q.q1 * q.q1 - q.q2 * q.q2 + q.q3 * q.q3);
    *pitch = asin(-2 * (q.q1 * q.q3 - q.q0 * q.q2));
    *yaw = atan2(2 * (q.q1 * q.q2 + q.q0 * q.q3), q.q0 * q.q0 + q.q1 * q.q1 - q.q2 * q.q2 - q.q3 * q.q3);
 
    *roll *= RtA;  // ����ת�Ƕ�
    *pitch *= RtA;
    *yaw *= RtA;
}

void GetAngles(float *pitch, float *roll, float *yaw, float dt) 
{
    float ax, ay, az, gx, gy, gz;
    MPU6050_ReadSensors(&ax, &ay, &az, &gx, &gy, &gz);
 
    NormalizeAccel(&ax, &ay, &az);
 
    float error_x, error_y, error_z;
    ComputeError(ax, ay, az, &error_x, &error_y, &error_z);
 
    UpdateQuaternion(gx, gy, gz, error_x, error_y, error_z, dt);
 
    ComputeEulerAngles(pitch, roll, yaw);
}

