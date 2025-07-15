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
uint8_t arxdata;			//接收中断缓冲
uint32_t rxlen = 0;
bool rxready;
uint8_t rxresult;
char rxdata[RX_BUFFER_SIZE];
/************************************************************************************
 * @brief       HAL_UART_RxCpltCallback
 * @note        串口中断回调函数
 *							接受esp的数据
 * @param       TIM_HandleTypeDef *huart
 * @retval      无
*************************************************************************************/
// USART2接收中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
	if (!rxready)
	{
			return;
	}
	
	if(rxlen >= RX_BUFFER_SIZE)  //溢出判断
	{
		rxlen = 0;
		rxresult = RX_RESULT_FAIL;
		rxready = false;
		memset(rxdata,0x00,sizeof(rxdata)); 
	}
	else
	{
		rxdata[rxlen++] = arxdata;   //接收数据转存
	
		if((rxdata[rxlen-1] == 0x0A)&&(rxdata[rxlen-2] == 0x0D)&&(rxdata[rxlen-3] == 'K')&&(rxdata[rxlen-4] == 'O')) //判断结束位
		{
			printf("Receive: %s\n", rxdata);
			rxlen = 0;
			rxresult = RX_RESULT_OK;
			rxready = false;
		}
		else if ((rxdata[rxlen - 1] == 0x0A)&&(rxdata[rxlen-2] == 0x0D)&&(rxdata[rxlen-3] == 'R')&&(rxdata[rxlen-4] == 'O')
					&&(rxdata[rxlen-5] == 'R')&&(rxdata[rxlen-6] == 'R')&&(rxdata[rxlen-7] == 'E'))
		{
				printf("接收ERROR：%s\r\n", rxdata);
				rxlen = 0;
				rxresult = RX_RESULT_ERROR;
				rxready = false;
				memset(rxdata,0x00,sizeof(rxdata)); //清空数组ERROR
		}
	}
	
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&arxdata, 1);   //再开启接收中断
}

/**
 * @brief       esp32初始化函数
 * @param       无
 * @retval      无
 */
bool esp_at_init(void)
{
    rxlen = 0;
		rxready = false;

    HAL_UART_Receive_IT(&huart2, (uint8_t *)&arxdata, 1);  // 开启串口中断接收
		
		return esp_at_reset(); 
}

/**
 * @brief       发送AT指令并等待响应
 * @param       const char *cmd AT指令
								uint32_t timeout 最大等待时间
 * @retval      0：发送成功 1：发送失败
 */
bool esp_at_send_command(const char *cmd, const char **rsp, uint32_t *length, uint32_t timeout)
{
    rxlen = 0;
    rxready = true;
    rxresult = RX_RESULT_FAIL;

		HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
		
    while (rxready && timeout--)
    {
        HAL_Delay(1);
    }
    rxready = false;

    if (rsp)
    {
        *rsp = (const char *)rxdata;
    }
    if (length)
    {
        *length = rxlen;
    }

    return rxresult == RX_RESULT_OK;
}

/**
 * @brief       ESP32C3 AT初始化
 * @param       无
 * @retval      1：初始化成功 0：初始化失败
 */
bool esp_at_reset(void)
{
		// 复位esp32
    if (!esp_at_send_command("AT\r\n", NULL, NULL, 1000))
    {
        return false;
    }
    HAL_Delay(2000);
    // 关闭回显
    if (!esp_at_send_command("ATE0\r\n", NULL, NULL, 1000))
    {
        return false;
    }
    // 关闭存储
    if (!esp_at_send_command("AT+SYSSTORE=0\r\n", NULL, NULL, 1000))
    {		
        return false;
    }

    return true;
}

/**
 * @brief       ESP32C3 WIFI初始化
 * @param       无
 * @retval      0：初始化成功 1：初始化失败
 */
bool esp_at_wifi_init(void)
{
    // 设置为station模式
    if (!esp_at_send_command("AT+CWMODE=1\r\n", NULL, NULL, 1000))
    {
        return false;
    }

    return true;
}

bool esp_at_wifi_connect(const char *ssid, const char *pwd)
{
    char cmd[64];

    // 连接wifi
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    if (!esp_at_send_command(cmd, NULL, NULL, 10000))
    {
        return false;
    }

    return true;
}

bool esp_at_http_get(const char *url, const char **rsp, uint32_t *length, uint32_t timeout)
{
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "AT+HTTPCGET=\"%s\"\r\n", url);
    if (!esp_at_send_command(cmd, rsp, length, 10000))
    {
        return false;
    }

    return true;
}

bool esp_at_sntp_init(void)
{
    // 设置为SNTP模式
    if (!esp_at_send_command("AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n", NULL, NULL, 1000))
    {
        return false;
    }

    // 查询sntp时间
    if (!esp_at_send_command("AT+CIPSNTPTIME?\r\n", NULL, NULL, 1000))
    {
        return false;
    }

    return true;
}

bool esp_at_time_get(uint32_t *timestamp)
{
    const char *rsp;
    uint32_t length;

    if (!esp_at_send_command("AT+SYSTIMESTAMP?\r\n", &rsp, &length, 1000))
    {
        return false;
    }

    char *sts = strstr(rsp, "+SYSTIMESTAMP:") + strlen("+SYSTIMESTAMP:");

    *timestamp = atoi(sts);

    return true;
}

bool weather_parse(const char *data, weather_t *weather)
{
    char *p = strstr(data, "\"text\":\"");
    if (p == NULL)
    {
        return false;
    }
    p += strlen("\"text\":\"");
    char *q = strchr(p, '\"');
    if (q == NULL)
    {
        return false;
    }
    int len = q - p;
    if (len >= sizeof(weather->weather))
    {
        len = sizeof(weather->weather) - 1;
    }
    strncpy(weather->weather, p, len);
    weather->weather[len] = '\0';

    p = strstr(data, "\"temperature\":\"");
    if (p == NULL)
    {
        return false;
    }
    p += strlen("\"temperature\":\"");
    q = strchr(p, '\"');
    if (q == NULL)
    {
        return false;
    }
    len = q - p;
    if (len >= sizeof(weather->temperature))
    {
        len = sizeof(weather->temperature) - 1;
    }
    strncpy(weather->temperature, p, len);
    weather->temperature[len] = '\0';

    return true;
}

lv_obj_t *label_city;
lv_obj_t *label_weather;
lv_obj_t *label_temperature;
lv_obj_t *label_time;

void ui_create(void)
{
    label_city = lv_label_create(lv_scr_act());
    lv_label_set_text(label_city, "City:	Xi'an");
    lv_obj_align(label_city, LV_ALIGN_TOP_LEFT, 10, 10);

    label_weather = lv_label_create(lv_scr_act());
    lv_label_set_text(label_weather, "Weather:");
    lv_obj_align(label_weather, LV_ALIGN_TOP_LEFT, 10, 40);

		label_temperature = lv_label_create(lv_scr_act());
    lv_label_set_text(label_temperature, "Temperature:");
    lv_obj_align(label_temperature, LV_ALIGN_TOP_LEFT, 10, 70);
	
    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "Time:");
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 10, 100);
}

void ui_update_weather_info(const weather_t *info)
{
    lv_label_set_text_fmt(label_weather, "Weather: %s", info->weather);
	  lv_label_set_text_fmt(label_temperature, "Temperature: %s", info->temperature);
    lv_label_set_text_fmt(label_time, "Time: %s", "11");
}
