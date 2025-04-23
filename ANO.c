#include "ANO.h"

extern UART_HandleTypeDef huart1;   //声明串口

// 将float转为int32_t（缩放后）
int32_t float_to_scaled_int(float value) {
    return (int32_t)(value * SCALE_FACTOR);
}

// 发送MPU6050传感器数据帧（功能字0xF2）波形绘制 6 个 int32_t
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // 帧头
    frame[frame_len++] = HEAD_ADDR;
    sum += HEAD_ADDR;

    // 设备地址
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // 功能字（自定义为0xF2，区别于其他帧）
    frame[frame_len++] = USERDATA2;
    sum += USERDATA2;

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

// 发送波形数据帧（功能字0xF1） 一个 uint16_t
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // 帧头
    frame[frame_len++] = HEAD_ADDR;
    sum += HEAD_ADDR;

    // 设备地址
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // 功能字
    frame[frame_len++] = USERDATA1;
    sum += USERDATA1;

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

