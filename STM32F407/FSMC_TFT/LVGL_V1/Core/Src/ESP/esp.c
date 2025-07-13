/******************************************************************************************************
 * @file        esp.c
 * @author      NingLi
 * @version     V1.0
 * @date        2025-7-13
 * @brief       esp32c3 AT指令 驱动代码
 * @license     提前在ESP官网烧录AT指令固件
 *****************************************************************************************************/
#include "esp.h"
#include <time.h>
#include <stdlib.h>

/* 变量 */
static uint32_t rxdata[RX_BUFFER_SIZE];
static uint32_t rxlen;
static uint8_t rxready;
static uint8_t rxresult;

/**
 * @brief       esp32初始化函数
 * @param       无
 * @retval      1：初始化成功 0：失败
 */
uint8_t esp_at_init(void)
{
    rxlen = 0;
    rxready = 0;
    rxresult = AT_RESULT_ERROR;

    HAL_UART_Receive_IT(&huart2, (uint8_t *)&rxdata[rxlen], 1);  // 开启串口中断接收
    return AT_RESULT_OK;
}

/**
 * @brief       发送AT指令并等待响应
 * @param       const char *cmd AT指令
								const char **rsp 回应
								uint32_t *length 回应长度
								uint32_t timeout 最大等待时间
 * @retval      1：初始化成功 0：失败
 */
uint8_t esp_at_send_command(const char *cmd, const char **rsp, uint32_t *length, uint32_t timeout)
{
    rxlen = 0;
    rxready = 0;

    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 1000);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 1000);

    uint32_t tickstart = HAL_GetTick();
    while (!rxready)
    {
        if (HAL_GetTick() - tickstart > timeout)
        {
            return AT_RESULT_FAIL;
        }
    }

    if (rsp) *rsp = (char *)rxdata;
    if (length) *length = rxlen;

    return rxresult;
}

uint8_t esp_at_send_data(const char *data, uint32_t *length)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)data, strlen(data), 1000);
    if (length) *length = strlen(data);
    return AT_RESULT_OK;
}

uint8_t esp_at_reset(void)
{
    const char *rsp;
    uint32_t len;
    return esp_at_send_command("AT+RST", &rsp, &len, 2000);
}

uint8_t esp_at_wifi_init(void)
{
    const char *rsp;
    uint32_t len;

    if (esp_at_send_command("ATE0", &rsp, &len, 1000) != AT_RESULT_OK) return AT_RESULT_FAIL; // 关闭回显
    if (esp_at_send_command("AT+CWMODE=1", &rsp, &len, 1000) != AT_RESULT_OK) return AT_RESULT_FAIL; // 设置为STA模式
    return AT_RESULT_OK;
}

uint8_t esp_at_wifi_connect(const char *ssid, const char *pwd)
{
    char cmd[128];
    const char *rsp;
    uint32_t len;

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    return esp_at_send_command(cmd, &rsp, &len, 10000);
}

uint8_t esp_at_http_get(const char *url, const char **rsp, uint32_t *length, uint32_t timeout)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+HTTPCGET=\"%s\"", url);
    return esp_at_send_command(cmd, rsp, length, timeout);
}

uint8_t esp_at_time_get(uint32_t *timestamp)
{
    const char *rsp;
    uint32_t len;
    if (esp_at_send_command("AT+SYSTIMESTAMP?", &rsp, &len, 2000) != AT_RESULT_OK)
        return AT_RESULT_FAIL;

    char *p = strstr(rsp, "+SYSTIMESTAMP:");
    if (p)
    {
        *timestamp = atoi(p + strlen("+SYSTIMESTAMP:"));
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAIL;
}

void format_time_string(uint32_t timestamp, char *time_str, size_t max_len)
{
    time_t rawtime = timestamp;
    struct tm *t = gmtime(&rawtime);  // 如果是本地时间用 localtime()

    snprintf(time_str, max_len, "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

lv_obj_t *label_city;
lv_obj_t *label_weather;
lv_obj_t *label_temp;
lv_obj_t *label_time;

void ui_create(void)
{
    label_city = lv_label_create(lv_scr_act());
    lv_label_set_text(label_city, "City:");
    lv_obj_align(label_city, LV_ALIGN_TOP_LEFT, 10, 10);

    label_weather = lv_label_create(lv_scr_act());
    lv_label_set_text(label_weather, "Weather:");
    lv_obj_align(label_weather, LV_ALIGN_TOP_LEFT, 10, 40);

    label_temp = lv_label_create(lv_scr_act());
    lv_label_set_text(label_temp, "Temp:");
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 10, 70);

    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "Time:");
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 10, 100);
}

void ui_update_weather_info(const WeatherInfo *info)
{
    char buf[64];
    
    lv_label_set_text_fmt(label_city, "City: %s", info->city);
    lv_label_set_text_fmt(label_weather, "Weather: %s", info->weather_text);
    lv_label_set_text_fmt(label_temp, "Temp: %s°C", info->temperature);

    format_time_string(info->timestamp, buf, sizeof(buf));
    lv_label_set_text_fmt(label_time, "Time: %s", buf);
}

void parse_weather_response(const char *json, WeatherInfo *info)
{
    // 非正式方法，仅适用于固定格式
    char *p;

    // 获取城市
    p = strstr(json, "\"name\":\"");
    if (p) sscanf(p, "\"name\":\"%[^\"]\"", info->city);

    // 获取天气
    p = strstr(json, "\"text\":\"");
    if (p) sscanf(p, "\"text\":\"%[^\"]\"", info->weather_text);

    // 获取温度
    p = strstr(json, "\"temperature\":\"");
    if (p) sscanf(p, "\"temperature\":\"%[^\"]\"", info->temperature);
}

WeatherInfo g_weather_info;
void weather_task(void)
{
    const char *rsp;
    uint32_t len;

    if (esp_at_http_get("https://api.seniverse.com/v3/weather/now.json?key=SZuZdJEYI7cY_ghfz&location=xian&language=en&unit=c", &rsp, &len, 5000) == AT_RESULT_OK)
    {
        parse_weather_response(rsp, &g_weather_info);
    }

    if (esp_at_time_get(&g_weather_info.timestamp) == AT_RESULT_OK)
    {
        ui_update_weather_info(&g_weather_info);
    }
}

