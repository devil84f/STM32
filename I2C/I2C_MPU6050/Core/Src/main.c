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
#include "tim.h"
#include "usart.h"
#include "gpio.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MPU6050_driver.h"
#include "oled.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
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
#define RXBUFFERSIZE  256
char RxBuffer[RXBUFFERSIZE]; 

#define SENDER_ADDR   0x05
#define TARGET_ADDR   0xAF
#define SCALE_FACTOR  1000  // 缩放因子（保留3位小数）

// 待发送的float数据
float accel[3] = {1.23f, -0.45f, 9.81f};  // 示例加速度计数据 (m/s2)
float gyro[3] = {0.78f, -1.56f, 3.14f};   // 示例陀螺仪数据 (rad/s)

// 将float转为int32_t（缩放后）
int32_t float_to_scaled_int(float value) {
    return (int32_t)(value * SCALE_FACTOR);
}

// 发送传感器数据帧（功能字0xF1）
HAL_StatusTypeDef SendSensorData(float *accel, float *gyro) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // 帧头
    frame[frame_len++] = 0xAA;
    sum += 0xAA;

    // 设备地址
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // 功能字（自定义为0xF2，区别于其他帧）
    frame[frame_len++] = 0xF2;
    sum += 0xF2;

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

// 发送波形数据帧（功能字0xF1）
HAL_StatusTypeDef SendWaveformData(uint16_t *data, uint16_t num) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    uint8_t sum = 0;

    // 帧头
    frame[frame_len++] = 0xAA;
    sum += 0xAA;

    // 设备地址
    frame[frame_len++] = SENDER_ADDR;
    sum += SENDER_ADDR;
    frame[frame_len++] = TARGET_ADDR;
    sum += TARGET_ADDR;

    // 功能字
    frame[frame_len++] = 0xF1;
    sum += 0xF1;

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
  MX_TIM1_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	// 初始化OLED
	OLED_Init();

	OLED_Clear();
	OLED_ShowString(1, 1, "g");    // 左栏标题
	OLED_ShowString(1, 9, "dps");   // 右栏标题

	// 初始化MPU6050（可选）
	if(MPU6050_Init(&hi2c2) != 0)
	{
		OLED_ShowString(3, 1, "MPU6050 Error!");
		while(1);
	}

	MPU6050_Data mpu_data;
	float accel[3], gyro[3];
	uint16_t A[2] = {0};
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	// 读取MPU6050数据（可选）
        MPU6050_Read_All(&hi2c2, &mpu_data);
        MPU6050_Convert_To_Real(&mpu_data, accel, gyro);
        
        // 显示加速度数据
        char buf[16];
        sprintf(buf, "g:%.2f", accel[0]);
        OLED_ShowString(2, 1, buf);
		sprintf(buf, "g:%.2f", accel[1]);
        OLED_ShowString(3, 1, buf);
		sprintf(buf, "g:%.2f", accel[2]);
        OLED_ShowString(4, 1, buf);

        
        // 显示陀螺仪数据
        sprintf(buf, "d:%.2f", gyro[0]);
        OLED_ShowString(2, 9, buf);
		sprintf(buf, "d:%.2f", gyro[1]);
        OLED_ShowString(3, 9, buf);
		sprintf(buf, "d:%.2f", gyro[2]);
        OLED_ShowString(4, 9, buf);
		
		SendSensorData(accel, gyro);
        // SendWaveformData(A, 2);
		A[0]++;
		A[1] += 2;
        HAL_Delay(100);  // 100ms刷新
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

