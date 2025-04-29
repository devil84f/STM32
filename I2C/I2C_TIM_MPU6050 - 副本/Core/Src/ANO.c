#include "ANO.h"

extern UART_HandleTypeDef huart1;   //声明串口

// 将float转为int32_t（缩放后）
int32_t float_to_scaled_int(float value) {
    return (int32_t)(value * SCALE_FACTOR);
}

// 发送传感器数据帧（功能字0xF1）
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // 帧头
    frame[frame_len++] = 0xAA;
    sum += 0xAA;

    // 设备地址
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // 功能字（自定义为0xF2，区别于其他帧）
    frame[frame_len++] = 0xF2;
    sum += 0xF2;

    // 数据部分：accel[3] + gyro[3]（每个float转为int32_t，占4字节）
    uint8_t data_bytes = 6 * sizeof(int32_t);  // 6个int32_t
    frame[frame_len++] = data_bytes;
    sum += data_bytes;

    // 填充加速度计数据（大端序）
    for (int i = 0; i < 3; i++) {
        int32_t scaled_accel = float_to_scaled_int(accel[i]);
        frame[frame_len++] = (scaled_accel >> 24) & 0xFF;
        frame[frame_len++] = (scaled_accel >> 16) & 0xFF;
        frame[frame_len++] = (scaled_accel >> 8) & 0xFF;
        frame[frame_len++] = scaled_accel & 0xFF;
        sum += frame[frame_len - 4] + frame[frame_len - 3] + frame[frame_len - 2] + frame[frame_len - 1];
    }

    // 填充陀螺仪数据（大端序）
    for (int i = 0; i < 3; i++) {
        int32_t scaled_gyro = float_to_scaled_int(gyro[i]);
        frame[frame_len++] = (scaled_gyro >> 24) & 0xFF;
        frame[frame_len++] = (scaled_gyro >> 16) & 0xFF;
        frame[frame_len++] = (scaled_gyro >> 8) & 0xFF;
        frame[frame_len++] = scaled_gyro & 0xFF;
        sum += frame[frame_len - 4] + frame[frame_len - 3] + frame[frame_len - 2] + frame[frame_len - 1];
    }

    // 校验和
    frame[frame_len++] = sum & 0xFF;

    // 发送数据
    return HAL_UART_Transmit(&huart1, frame, frame_len, HAL_MAX_DELAY);
}

// 发送波形数据帧（功能字0xF1）
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // 帧头
    frame[frame_len++] = 0xAA;
    sum += 0xAA;

    // 设备地址
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // 功能字
    frame[frame_len++] = 0xF1;
    sum += 0xF1;

    // 数据长度（每个uint16_t拆分为2字节）
    uint16_t data_bytes = num * 2;
    frame[frame_len++] = (uint8_t)data_bytes;
    sum += (uint8_t)data_bytes;

    // 填充数据（大端序）
    for (uint16_t i = 0; i < num; i++) {
        frame[frame_len++] = (data[i] >> 8) & 0xFF;  // 高字节
        sum += frame[frame_len - 1];
        frame[frame_len++] = data[i] & 0xFF;         // 低字节
        sum += frame[frame_len - 1];
    }

    // 校验和
    frame[frame_len++] = sum & 0xFF;

    // 发送数据
    return HAL_UART_Transmit(&huart1, frame, frame_len, HAL_MAX_DELAY);
}

HAL_StatusTypeDef Send_ANOTC(short pitch, short roll, short yaw) 
{
    uint8_t tx_buffer[18]; // 正确长度：5B协议头 + 12B数据 + 1B校验 = 18B
    uint16_t idx = 0;

    // 1. 协议头（5字节）
    tx_buffer[idx++] = 0xAA;  // 0xAA
    tx_buffer[idx++] = SENDER_ADDR;       // 发送地址
    tx_buffer[idx++] = TARGET_ADDR;       // 目标地址
    tx_buffer[idx++] = 0x01;    // 功能字0x01
    tx_buffer[idx++] = 0x12;            // 数据长度LEN=12（仅数据部分）

    // 2. 数据部分（12字节）
	tx_buffer[idx++] = (uint8_t)(roll * 100);
	tx_buffer[idx++] = (uint8_t)((roll * 100)>>8);
	tx_buffer[idx++] = (uint8_t)(pitch * 100);
	tx_buffer[idx++] = (uint8_t)((pitch * 100)>>8);
	tx_buffer[idx++] = (uint8_t)(yaw * 100);
	tx_buffer[idx++] = (uint8_t)((yaw * 100)>>8);

    tx_buffer[idx++] = 0x00;
	tx_buffer[idx++] = 0x00;
	tx_buffer[idx++] = 0x00;
	tx_buffer[idx++] = 0x00;

    tx_buffer[idx++] = 0x00;    // FLY_MODEL
    tx_buffer[idx++] = 1;    // ARMED

    // 3. 校验和（从帧头0xAA到ARMED字节）
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < idx; i++) { // 注意这里包含协议头和数据部分
        checksum += tx_buffer[i];
    }
    tx_buffer[idx++] = checksum;

    // 4. 发送（18字节）    
    return HAL_UART_Transmit(&huart1, tx_buffer, idx, HAL_MAX_DELAY);
}

//详情参考匿名通讯协议
void SEND_OULA_ANGLE(short row, short pit, short yaw)
{
	uint8_t i;
	uint8_t sumcheck = 0;
	uint8_t addcheck = 0;
	uint8_t buf[13]={0};
	
	buf[0]=0xaa;
	buf[1]=0xff;
	buf[2]=0x03;
	buf[3]=0x07;
	
	buf[4]=(uint8_t)row;
	buf[5]=(uint8_t)(row>>8);
	
	buf[6]=(uint8_t)pit;
	buf[7]=(uint8_t)(pit>>8);

	buf[8]=(uint8_t)yaw;
	buf[9]=(uint8_t)(yaw>>8);
	
	buf[10]=0x00;
	
	for(i=0; i < (buf[3]+4); i++)
	{
		sumcheck += buf[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
		addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
	}
	buf[11]=sumcheck;
	buf[12]=addcheck;
	
	HAL_UART_Transmit(&huart1, buf, 13, HAL_MAX_DELAY);
}

