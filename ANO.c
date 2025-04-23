#include "ANO.h"

extern UART_HandleTypeDef huart1;   //��������

// ��floatתΪint32_t�����ź�
int32_t float_to_scaled_int(float value) {
    return (int32_t)(value * SCALE_FACTOR);
}

// ����MPU6050����������֡��������0xF2�����λ��� 6 �� int32_t
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // ֡ͷ
    frame[frame_len++] = HEAD_ADDR;
    sum += HEAD_ADDR;

    // �豸��ַ
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // �����֣��Զ���Ϊ0xF2������������֡��
    frame[frame_len++] = USERDATA2;
    sum += USERDATA2;

    // ���ݲ��֣�accel[3] + gyro[3]��ÿ��floatתΪint32_t��ռ4�ֽڣ�
    uint8_t data_bytes = 6 * sizeof(int32_t);  // 6��int32_t
    frame[frame_len++] = data_bytes;
    sum += data_bytes;

    // �����ٶȼ����ݣ������
    for (int i = 0; i < 3; i++) {
        int32_t scaled_accel = float_to_scaled_int(accel[i]);
        frame[frame_len++] = (scaled_accel >> 24) & 0xFF;
        frame[frame_len++] = (scaled_accel >> 16) & 0xFF;
        frame[frame_len++] = (scaled_accel >> 8) & 0xFF;
        frame[frame_len++] = scaled_accel & 0xFF;
        sum += frame[frame_len - 4] + frame[frame_len - 3] + frame[frame_len - 2] + frame[frame_len - 1];
    }

    // ������������ݣ������
    for (int i = 0; i < 3; i++) {
        int32_t scaled_gyro = float_to_scaled_int(gyro[i]);
        frame[frame_len++] = (scaled_gyro >> 24) & 0xFF;
        frame[frame_len++] = (scaled_gyro >> 16) & 0xFF;
        frame[frame_len++] = (scaled_gyro >> 8) & 0xFF;
        frame[frame_len++] = scaled_gyro & 0xFF;
        sum += frame[frame_len - 4] + frame[frame_len - 3] + frame[frame_len - 2] + frame[frame_len - 1];
    }

    // У���
    frame[frame_len++] = sum & 0xFF;

    // ��������
    return HAL_UART_Transmit(&huart1, frame, frame_len, HAL_MAX_DELAY);
}

// ���Ͳ�������֡��������0xF1�� һ�� uint16_t
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // ֡ͷ
    frame[frame_len++] = HEAD_ADDR;
    sum += HEAD_ADDR;

    // �豸��ַ
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // ������
    frame[frame_len++] = USERDATA1;
    sum += USERDATA1;

    // ���ݳ��ȣ�ÿ��uint16_t���Ϊ2�ֽڣ�
    uint16_t data_bytes = num * 2;
    frame[frame_len++] = (uint8_t)data_bytes;
    sum += (uint8_t)data_bytes;

    // ������ݣ������
    for (uint16_t i = 0; i < num; i++) {
        frame[frame_len++] = (data[i] >> 8) & 0xFF;  // ���ֽ�
        sum += frame[frame_len - 1];
        frame[frame_len++] = data[i] & 0xFF;         // ���ֽ�
        sum += frame[frame_len - 1];
    }

    // У���
    frame[frame_len++] = sum & 0xFF;

    // ��������
    return HAL_UART_Transmit(&huart1, frame, frame_len, HAL_MAX_DELAY);
}

