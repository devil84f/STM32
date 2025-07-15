/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "lcd.h"
#include "touch.h"

#include "esp.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
forѭ��ʵ����ʱus
*/
void for_delay_us(uint32_t nus)
{
    uint32_t Delay = nus * 168/4;
    do
    {
        __NOP();
    }
    while (Delay --);
}

static const char *wifi_ssid = "RK50U";
static const char *wifi_password = "12345678";
static const char *weather_uri = "https://api.seniverse.com/v3/weather/now.json?key=SZuZdJEYI7cY_ghfz&location=xian&language=en&unit=c";
static const char *time_uri = "https://time.akamai.com/";
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_FSMC_Init();
  MX_SPI2_Init();
  MX_I2C1_Init();
  MX_TIM6_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
	// LCD ��������ʼ��
	lcd_init(); 
	g_point_color = RED;
  //tp_dev.init();
	
	// LVGL��ʼ��
	lv_init();                             // LVGL ��ʼ��
	lv_port_disp_init();                   // ע��LVGL����ʾ����
	//lv_port_indev_init();                  // ע��LVGL�Ĵ����������
	
	// ����LVGLʱ��
	HAL_TIM_Base_Start_IT(&htim6);
	ui_create();              // ������ʼ����
	lv_timer_handler();				// LVGL �ڲ�������
	if (!esp_at_init())
	{
			while (1);
	}
	HAL_Delay(500);
	if (!esp_at_wifi_init())
	{
			while (1);
	}
	HAL_Delay(500);
	if (!esp_at_wifi_connect(wifi_ssid, wifi_password))
	{
			printf("Dead in esp_at_wifi_connect\r\n");
			while (1);
	}
	bool weather_ok = false;
	bool sntp_ok = false;
	uint32_t t = 0;
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	char str[64];
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		lv_task_handler();    // LVGL �ڲ�������
		HAL_Delay(5);
		
		t++;
    HAL_Delay(1000);
		
		/* BUG:�������Ϊ20��10������20sʱ������ͬʱ�������ж������������ͻ��������� */
		if (!weather_ok || t % 20 == 0)
		{
				const char *rsp;
				weather_ok = esp_at_http_get(weather_uri, &rsp, NULL, 10000);
				weather_t weather;
			
				weather_parse(rsp, &weather);
				
				memset(rxdata,0x00,sizeof(rxdata)); //�������ERROR
				ui_update_weather_info(&weather);
		}
		if (!sntp_ok || t % 15 == 0)
		{
				const char *rsp1;
				sntp_ok = esp_at_http_get(time_uri, &rsp1, NULL, 10000);
				const char *comma_pos = strrchr(rsp1, ','); // �ҵ����һ�����ŵ�λ��
				const char *number;
				if (comma_pos != NULL) {
						number = comma_pos + 1; // ��������
				}
				uint32_t UnixTime = atoi(number);
				rtc_date_t date;
        ts_to_date(UnixTime + 8 * 60 * 60, &date);
        snprintf(str, sizeof(str), "%04d-%02d-%02d  %02d:%02d:%02d", date.year, date.month, date.day, date.hour, date.minute, date.second);
				printf("%s\r\n", str);
				ui_update_time_info(str);
		}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/************************************************************************************
 * @brief       HAL_TIM_PeriodElapsedCallback
 * @note        TIM6�ص�����
 *							ΪLVGL�ṩ1ms���������ڣ�����ʱ������Ϊ2ms����жϣ���lv_tick_inc(2);
 * @param       TIM_HandleTypeDef *htim
 * @retval      ��
*************************************************************************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) 
{
	if (htim->Instance == TIM6) // �ж��ж�����TIM6
	{
		lv_tick_inc(1); // ��LVGL�ṩ1ms��������������
		static uint16_t LedTimes = 0;  // LED��˸��ʱ
		if (LedTimes++ >= 500) //500msִ��һ��LED��˸
		{
			HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
			LedTimes = 0;
		}
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

