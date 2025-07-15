/******************************************************************************************************
 * @file        esp.c
 * @author      NingLi
 * @version     V1.0
 * @date        2025-7-13
 * @brief       esp32c3 ATָ�� ��������
 * @license     ��ǰ��ESP������¼ATָ��̼�
 *****************************************************************************************************/
#include "esp.h"
#include <time.h>
#include <stdlib.h>

/* ���� */
uint8_t arxdata;			//�����жϻ���
uint32_t rxlen = 0;
bool rxready;
uint8_t rxresult;
char rxdata[RX_BUFFER_SIZE];
/************************************************************************************
 * @brief       HAL_UART_RxCpltCallback
 * @note        �����жϻص�����
 *							����esp������
 * @param       TIM_HandleTypeDef *huart
 * @retval      ��
*************************************************************************************/
// USART2�����жϻص�
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
	
	if(rxlen >= RX_BUFFER_SIZE)  //����ж�
	{
		rxlen = 0;
		rxresult = RX_RESULT_FAIL;
		rxready = false;
		memset(rxdata,0x00,sizeof(rxdata)); 
	}
	else
	{
		rxdata[rxlen++] = arxdata;   //��������ת��
	
		if((rxdata[rxlen-1] == 0x0A)&&(rxdata[rxlen-2] == 0x0D)&&(rxdata[rxlen-3] == 'K')&&(rxdata[rxlen-4] == 'O')) //�жϽ���λ
		{
			printf("Receive: %s\n", rxdata);
			rxlen = 0;
			rxresult = RX_RESULT_OK;
			rxready = false;
		}
		else if ((rxdata[rxlen - 1] == 0x0A)&&(rxdata[rxlen-2] == 0x0D)&&(rxdata[rxlen-3] == 'R')&&(rxdata[rxlen-4] == 'O')
					&&(rxdata[rxlen-5] == 'R')&&(rxdata[rxlen-6] == 'R')&&(rxdata[rxlen-7] == 'E'))
		{
				printf("����ERROR��%s\r\n", rxdata);
				rxlen = 0;
				rxresult = RX_RESULT_ERROR;
				rxready = false;
				memset(rxdata,0x00,sizeof(rxdata)); //�������ERROR
		}
	}
	
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&arxdata, 1);   //�ٿ��������ж�
}

/**
 * @brief       esp32��ʼ������
 * @param       ��
 * @retval      ��
 */
bool esp_at_init(void)
{
    rxlen = 0;
		rxready = false;

    HAL_UART_Receive_IT(&huart2, (uint8_t *)&arxdata, 1);  // ���������жϽ���
		
		return esp_at_reset(); 
}

/**
 * @brief       ����ATָ��ȴ���Ӧ
 * @param       const char *cmd ATָ��
								uint32_t timeout ���ȴ�ʱ��
 * @retval      0�����ͳɹ� 1������ʧ��
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
 * @brief       ESP32C3 AT��ʼ��
 * @param       ��
 * @retval      1����ʼ���ɹ� 0����ʼ��ʧ��
 */
bool esp_at_reset(void)
{
		// ��λesp32
    if (!esp_at_send_command("AT\r\n", NULL, NULL, 1000))
    {
        return false;
    }
    HAL_Delay(2000);
    // �رջ���
    if (!esp_at_send_command("ATE0\r\n", NULL, NULL, 1000))
    {
        return false;
    }
    // �رմ洢
    if (!esp_at_send_command("AT+SYSSTORE=0\r\n", NULL, NULL, 1000))
    {		
        return false;
    }

    return true;
}

/**
 * @brief       ESP32C3 WIFI��ʼ��
 * @param       ��
 * @retval      0����ʼ���ɹ� 1����ʼ��ʧ��
 */
bool esp_at_wifi_init(void)
{
    // ����Ϊstationģʽ
    if (!esp_at_send_command("AT+CWMODE=1\r\n", NULL, NULL, 1000))
    {
        return false;
    }

    return true;
}

bool esp_at_wifi_connect(const char *ssid, const char *pwd)
{
    char cmd[64];

    // ����wifi
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
    // ����ΪSNTPģʽ
    if (!esp_at_send_command("AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n", NULL, NULL, 1000))
    {
        return false;
    }

    // ��ѯsntpʱ��
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
