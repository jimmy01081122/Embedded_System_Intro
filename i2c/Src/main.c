/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2026 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_hal.h"
#include <stdio.h>
#include <string.h>
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define ADXL345_ADDR   (0x53 << 1)
#define I2C_TIMEOUT    100
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* 寫入單一寄存器 */
HAL_StatusTypeDef ADXL345_I2C_WriteReg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = { reg, data };
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        &hi2c1, ADXL345_ADDR, buf, 2, I2C_TIMEOUT);
    if (ret != HAL_OK) {
        printf("[I2C ERR] WriteReg 0x%02X failed, HAL=%d\r\n", reg, ret);
    }
    return ret;
}

/* 讀取單一寄存器 */
uint8_t ADXL345_I2C_ReadReg(uint8_t reg) {
    uint8_t val = 0;
    HAL_StatusTypeDef ret;

    /* 先送出寄存器地址 */
    ret = HAL_I2C_Master_Transmit(
        &hi2c1, ADXL345_ADDR, &reg, 1, I2C_TIMEOUT);
    if (ret != HAL_OK) {
        printf("[I2C ERR] ReadReg Tx 0x%02X failed, HAL=%d\r\n", reg, ret);
        return 0;
    }

    /* 再讀取 1 byte */
    ret = HAL_I2C_Master_Receive(
        &hi2c1, ADXL345_ADDR, &val, 1, I2C_TIMEOUT);
    if (ret != HAL_OK) {
        printf("[I2C ERR] ReadReg Rx 0x%02X failed, HAL=%d\r\n", reg, ret);
        return 0;
    }
    return val;
}

/* 更簡潔的寫法：使用 Mem_Read / Mem_Write（HAL 內建） */
HAL_StatusTypeDef ADXL345_I2C_MemWrite(uint8_t reg, uint8_t data) {
    return HAL_I2C_Mem_Write(
        &hi2c1, ADXL345_ADDR,
        reg, I2C_MEMADD_SIZE_8BIT,
        &data, 1, I2C_TIMEOUT);
}

uint8_t ADXL345_I2C_MemRead(uint8_t reg) {
    uint8_t val = 0;
    HAL_I2C_Mem_Read(
        &hi2c1, ADXL345_ADDR,
        reg, I2C_MEMADD_SIZE_8BIT,
        &val, 1, I2C_TIMEOUT);
    return val;
}

/* 連續讀取多個寄存器（Burst read） */
HAL_StatusTypeDef ADXL345_I2C_ReadMulti(uint8_t reg, uint8_t *buf, uint8_t len) {
    return HAL_I2C_Mem_Read(
        &hi2c1, ADXL345_ADDR,
        reg, I2C_MEMADD_SIZE_8BIT,
        buf, len, I2C_TIMEOUT);
}

void ADXL345_I2C_Init(void) {
    HAL_StatusTypeDef ret;

    /* 確認 Device ID */
    uint8_t id = ADXL345_I2C_MemRead(0x00);
    printf("[I2C] Device ID = 0x%02X %s\r\n",
           id, (id == 0xE5) ? "(OK)" : "(FAIL!)");

    /* DATA_FORMAT: Full resolution, ±2g */
    ret = ADXL345_I2C_MemWrite(0x31, 0x08);
    printf("[I2C] DATA_FORMAT write %s\r\n",
           (ret == HAL_OK) ? "OK" : "FAIL");

    /* BW_RATE: 100Hz */
    ADXL345_I2C_MemWrite(0x2C, 0x0A);

    /* POWER_CTL: Measure mode */
    ret = ADXL345_I2C_MemWrite(0x2D, 0x08);
    printf("[I2C] POWER_CTL (Measure) write %s\r\n",
           (ret == HAL_OK) ? "OK" : "FAIL");
}

void ADXL345_I2C_ReadXYZ(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t raw[6];
    HAL_StatusTypeDef ret = ADXL345_I2C_ReadMulti(0x32, raw, 6);

    if (ret != HAL_OK) {
        printf("[I2C ERR] ReadXYZ failed, HAL=%d\r\n", ret);
        *x = *y = *z = 0;
        return;
    }

    *x = (int16_t)((raw[1] << 8) | raw[0]);
    *y = (int16_t)((raw[3] << 8) | raw[2]);
    *z = (int16_t)((raw[5] << 8) | raw[4]);
}

void Debug_I2C_Error(HAL_StatusTypeDef ret, I2C_HandleTypeDef *hi2c) {
    char buf[80];
    const char *reason;

    switch (ret) {
        case HAL_ERROR:   reason = "HAL_ERROR (general)"; break;
        case HAL_BUSY:    reason = "HAL_BUSY (bus held)"; break;
        case HAL_TIMEOUT: reason = "HAL_TIMEOUT (no ACK?)"; break;
        default:          reason = "Unknown"; break;
    }

    /* 進一步看 ErrorCode */
    sprintf(buf, "[I2C] Status=%d (%s), ErrorCode=0x%08lX\r\n",
            ret, reason, hi2c->ErrorCode);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);

    /* 常見 ErrorCode:
       0x00000004 = AF (Address NACK) → 地址錯誤或裝置未回應
       0x00000008 = ARLO (Arbitration Lost)
       0x00000020 = TIMEOUT                */
}

void Debug_SprintfExample(int16_t x, int16_t y, int16_t z) {
    char buf[50] = {0};
    sprintf(buf, "[DATA] I2C raw X=%6d Y=%6d Z=%6d\r\n", x, y, z);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
}

void Debug_PrintXYZ(int16_t x, int16_t y, int16_t z) {
		char buf[100] = {0};

		int32_t gx_val = ((int32_t)x * 39) / 100;
	    int32_t gy_val = ((int32_t)y * 39) / 100;
	    int32_t gz_val = ((int32_t)z * 39) / 100;

	    /* 2. 判斷正負號 */
	    char sx2 = (gx_val < 0) ? '-' : '+';
	    char sy2 = (gy_val < 0) ? '-' : '+';
	    char sz2 = (gz_val < 0) ? '-' : '+';

	    /* 3. 取絕對值，轉為無號數 */
	    uint32_t ax2 = (gx_val < 0) ? -gx_val : gx_val;
	    uint32_t ay2 = (gy_val < 0) ? -gy_val : gy_val;
	    uint32_t az2 = (gz_val < 0) ? -gz_val : gz_val;

	    /* ax / 10 取出 mg 整數部分，ax % 10 取出小數點後第一位 */
	//    sprintf(buf, "[G]    X=%c%lu.%lu mg  Y=%c%lu.%lu mg  Z=%c%lu.%lu mg\r\n",
	//            sx, ax / 10, ax % 10,
	//            sy, ay / 10, ay % 10,
	//            sz, az / 10, az % 10);
	//    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), DELAY);
	    sprintf(buf, "[G]    X=%c%lu.%02lu g  Y=%c%lu.%02lu g  Z=%c%lu.%02lu g\r\n",
	                sx2, ax2 / 100, ax2 % 100,
	                sy2, ay2 / 100, ay2 % 100,
	                sz2, az2 / 100, az2 % 100);
	    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 1000);

//    char buf[100] = {0};
//
//    /* --- mg 輸出（1 LSB = 3.9 mg，用 39/10 整數近似）--- */
//    int32_t gx_mg = (int32_t)x * 39;
//    int32_t gy_mg = (int32_t)y * 39;
//    int32_t gz_mg = (int32_t)z * 39;
//
//    char sx = (gx_mg < 0) ? '-' : '+';
//    char sy = (gy_mg < 0) ? '-' : '+';
//    char sz = (gz_mg < 0) ? '-' : '+';
//    uint32_t ax = (gx_mg < 0) ? -gx_mg : gx_mg;
//    uint32_t ay = (gy_mg < 0) ? -gy_mg : gy_mg;
//    uint32_t az = (gz_mg < 0) ? -gz_mg : gz_mg;
//
//    sprintf(buf, "[mg]   X=%c%4lu.%lu  Y=%c%4lu.%lu  Z=%c%4lu.%lu\r\n",
//            sx, ax / 10, ax % 10,
//            sy, ay / 10, ay % 10,
//            sz, az / 10, az % 10);
//    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
//
//    /* --- g 輸出（除以 1000，保留 3 位小數）--- */
//    int32_t gx_val = (int32_t)x * 39 / 10;   /* 單位：0.01 mg */
//    int32_t gy_val = (int32_t)y * 39 / 10;
//    int32_t gz_val = (int32_t)z * 39 / 10;
//
//    char sx2 = (gx_val < 0) ? '-' : '+';
//    char sy2 = (gy_val < 0) ? '-' : '+';
//    char sz2 = (gz_val < 0) ? '-' : '+';
//    uint32_t ax2 = (gx_val < 0) ? -gx_val : gx_val;
//    uint32_t ay2 = (gy_val < 0) ? -gy_val : gy_val;
//    uint32_t az2 = (gz_val < 0) ? -gz_val : gz_val;
//
//    sprintf(buf, "[g]    X=%c%lu.%02lu  Y=%c%lu.%02lu  Z=%c%lu.%02lu\r\n",
//            sx2, ax2 / 100, ax2 % 100,
//            sy2, ay2 / 100, ay2 % 100,
//            sz2, az2 / 100, az2 % 100);
//    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
 	 {
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  int16_t ax, ay, az;
  ADXL345_I2C_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
	  ADXL345_I2C_ReadXYZ(&ax, &ay, &az);
	  //Debug_SprintfExample(ax, ay, az);
	  Debug_PrintXYZ(ax, ay, az);
	  HAL_Delay(1500); /* 根據 100Hz 輸出率，50ms 讀取一次為合理間距 */

  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Analogue filter 
    */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Digital filter 
    */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
