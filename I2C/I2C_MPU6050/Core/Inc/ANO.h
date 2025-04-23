#ifndef __ANO_H
#define __ANO_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
	 
#define SENDER_ADDR   0x05
#define TARGET_ADDR   0xAF
	 
#define SCALE_FACTOR  100  // 缩放因子（保留2位小数）

int32_t float_to_scaled_int(float value);
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro);
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num);
	 
#ifdef __cplusplus
}
#endif

#endif /* __ANO_H */
