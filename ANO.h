#ifndef __ANO_H
#define __ANO_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
	 
// 地址定义
#define SENDER_ADDR   0x05 // 发送设备
#define TARGET_ADDR   0xAF // 发送至上位机
#define HEAD_ADDR     0xAF // 帧头

// 功能字定义
#define VER     0x00 // 版本信息
#define STATUS     0x01 // 姿态
#define SENSER     0x02 // 传感器数据
#define RCDATA     0x03 // 控制数据
#define GPSDATA     0x04 // GPS
#define USERDATA1     0xF1 // 用户数据1
#define USERDATA2     0xF2 // 用户数据2
#define USERDATA3     0xF3 // 用户数据3
#define USERDATA4     0xF4 // 用户数据4
#define USERDATA5     0xF5 // 用户数据5
	 
#define SCALE_FACTOR  100  // 缩放因子（保留2位小数）

int32_t float_to_scaled_int(float value);
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro);
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num);
	 
#ifdef __cplusplus
}
#endif

#endif /* __ANO_H */
