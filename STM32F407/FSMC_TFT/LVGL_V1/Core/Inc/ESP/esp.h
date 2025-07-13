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

#define AT_RESULT_OK     0
#define AT_RESULT_ERROR  1
#define AT_RESULT_FAIL   2

typedef struct {
    char city[32];
    char weather_text[32];  // �� "Sunny"
    char temperature[8];    // �� "27"
    uint32_t timestamp;     // ʱ���
} WeatherInfo;

/* ���� */
static uint32_t rxdata[RX_BUFFER_SIZE];
static uint32_t rxlen;
static uint8_t rxready;
static uint8_t rxresult;

/* �������� */
uint8_t esp_at_init(void);
uint8_t esp_at_send_command(const char *cmd, const char **rsp, uint32_t *length, uint32_t timeout);
uint8_t esp_at_send_data(const char *data, uint32_t *length);

uint8_t esp_at_reset(void);

uint8_t esp_at_wifi_init(void);
uint8_t esp_at_wifi_connect(const char *ssid, const char *pwd);

uint8_t esp_at_http_get(const char *url, const char **rsp, uint32_t *length, uint32_t timeout);
uint8_t esp_at_time_get(uint32_t *timestamp);

void format_time_string(uint32_t timestamp, char *time_str, size_t max_len);
void ui_create(void);
void ui_update_weather_info(const WeatherInfo *info);
void parse_weather_response(const char *json, WeatherInfo *info);
void weather_task(void);

#endif /* __ESP_H__ */













