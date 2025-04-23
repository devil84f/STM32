#ifndef __ANO_H
#define __ANO_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
	 
// ��ַ����
#define SENDER_ADDR   0x05 // �����豸
#define TARGET_ADDR   0xAF // ��������λ��
#define HEAD_ADDR     0xAF // ֡ͷ

// �����ֶ���
#define VER     0x00 // �汾��Ϣ
#define STATUS     0x01 // ��̬
#define SENSER     0x02 // ����������
#define RCDATA     0x03 // ��������
#define GPSDATA     0x04 // GPS
#define USERDATA1     0xF1 // �û�����1
#define USERDATA2     0xF2 // �û�����2
#define USERDATA3     0xF3 // �û�����3
#define USERDATA4     0xF4 // �û�����4
#define USERDATA5     0xF5 // �û�����5
	 
#define SCALE_FACTOR  100  // �������ӣ�����2λС����

int32_t float_to_scaled_int(float value);
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro);
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num);
	 
#ifdef __cplusplus
}
#endif

#endif /* __ANO_H */
