/******************************************************************************************************
 * @file        esp.h
 * @author      NingLi
 * @version     V1.0
 * @date        2025-7-13
 * @brief       esp32c3 ATָ�� ��������
 * @license     ��ǰ��ESP������¼ATָ��̼�
 *****************************************************************************************************/
#ifndef __ESP_H__
#define __ESP_H__

/* ͷ�ļ� */
#include "stdio.h"
#include "string.h"
#include "main.h"
#include "usart.h"

/* �궨�� */
#define RX_BUFFER_SIZE   4096

#define RX_RESULT_OK    0
#define RX_RESULT_ERROR 1
#define RX_RESULT_FAIL  2

/* ���� */
extern uint8_t arxdata;			//�����жϻ���
extern uint32_t rxlen;
extern bool rxready;
extern uint8_t rxresult;
extern char rxdata[RX_BUFFER_SIZE];

/* �������� */
bool esp_at_init(void);
bool esp_at_send_command(const char *cmd, const char **rsp, uint32_t *length, uint32_t timeout);
bool esp_at_send_data(const uint8_t *data, uint32_t length);

bool esp_at_reset(void);

bool esp_at_wifi_init(void);
bool esp_at_wifi_connect(const char *ssid, const char *pwd);

bool esp_at_http_get(const char *url, const char **rsp, uint32_t *length, uint32_t timeout);
bool esp_at_sntp_init(void);
bool esp_at_time_get(uint32_t *timestamp);

typedef struct
{
    char weather[32];
    char temperature[8];
} weather_t;


bool weather_parse(const char *data, weather_t *weather);

void ui_create(void);
void ui_update_weather_info(const weather_t *info);

#endif /* __ESP_H__ */












