/******************************************************************************************************
 * @file        esp.h
 * @author      NingLi
 * @version     V1.0
 * @date        2025-7-13
 * @brief       esp32c3 AT指令 驱动代码
 * @license     提前在ESP官网烧录AT指令固件
 *****************************************************************************************************/
#ifndef __ESP_H__
#define __ESP_H__

/* 头文件 */
#include "stdio.h"
#include "string.h"
#include "main.h"
#include "usart.h"
#include "time.h"

/* 宏定义 */
#define RX_BUFFER_SIZE   4096

#define RX_RESULT_OK    0
#define RX_RESULT_ERROR 1
#define RX_RESULT_FAIL  2

/* 变量 */
extern uint8_t arxdata;			//接收中断缓冲
extern uint32_t rxlen;
extern bool rxready;
extern uint8_t rxresult;
extern char rxdata[RX_BUFFER_SIZE];

/* 函数声明 */
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
void ui_update_time_info(char *info);

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_date_t;
void ts_to_date(uint32_t seconds, rtc_date_t *date);
#endif /* __ESP_H__ */












