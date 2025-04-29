#include "ANO.h"

extern UART_HandleTypeDef huart1;   //��������

// ��floatתΪint32_t�����ź�
int32_t float_to_scaled_int(float value) {
    return (int32_t)(value * SCALE_FACTOR);
}

// ���ʹ���������֡��������0xF1��
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // ֡ͷ
    frame[frame_len++] = 0xAA;
    sum += 0xAA;

    // �豸��ַ
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // �����֣��Զ���Ϊ0xF2������������֡��
    frame[frame_len++] = 0xF2;
    sum += 0xF2;

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

// ���Ͳ�������֡��������0xF1��
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // ֡ͷ
    frame[frame_len++] = 0xAA;
    sum += 0xAA;

    // �豸��ַ
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // ������
    frame[frame_len++] = 0xF1;
    sum += 0xF1;

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

HAL_StatusTypeDef Send_ANOTC(short pitch, short roll, short yaw) 
{
    uint8_t tx_buffer[18]; // ��ȷ���ȣ�5BЭ��ͷ + 12B���� + 1BУ�� = 18B
    uint16_t idx = 0;

    // 1. Э��ͷ��5�ֽڣ�
    tx_buffer[idx++] = 0xAA;  // 0xAA
    tx_buffer[idx++] = SENDER_ADDR;       // ���͵�ַ
    tx_buffer[idx++] = TARGET_ADDR;       // Ŀ���ַ
    tx_buffer[idx++] = 0x01;    // ������0x01
    tx_buffer[idx++] = 0x12;            // ���ݳ���LEN=12�������ݲ��֣�

    // 2. ���ݲ��֣�12�ֽڣ�
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

    // 3. У��ͣ���֡ͷ0xAA��ARMED�ֽڣ�
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < idx; i++) { // ע���������Э��ͷ�����ݲ���
        checksum += tx_buffer[i];
    }
    tx_buffer[idx++] = checksum;

    // 4. ���ͣ�18�ֽڣ�    
    return HAL_UART_Transmit(&huart1, tx_buffer, idx, HAL_MAX_DELAY);
}

//����ο�����ͨѶЭ��
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
		sumcheck += buf[i]; //��֡ͷ��ʼ����ÿһ�ֽڽ�����ͣ�ֱ��DATA������
		addcheck += sumcheck; //ÿһ�ֽڵ���Ͳ���������һ��sumcheck���ۼ�
	}
	buf[11]=sumcheck;
	buf[12]=addcheck;
	
	HAL_UART_Transmit(&huart1, buf, 13, HAL_MAX_DELAY);
}

