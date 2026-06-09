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

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define ADXL345_CS_PORT GPIOB
#define ADXL345_CS_PIN  GPIO_PIN_12
#define ADXL345_CS_LOW()  HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_RESET)
#define ADXL345_CS_HIGH() HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_SET)
#define DELAY 50
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

void Debug_SprintfExample(int16_t x, int16_t y, int16_t z) {
    char buf[64];

    sprintf(buf, "[DATA] raw X=%6d Y=%6d Z=%6d\r\n", x, y, z);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf),  DELAY);

}

void ADXL345_WriteReg(uint8_t reg, uint8_t data) {
    uint8_t tx_buf[2];
    tx_buf[0] = reg & 0x3F; /* 確保 bit7(Read) 為 0, bit6(MB) 為 0 */
    tx_buf[1] = data;

    ADXL345_CS_LOW();
    HAL_SPI_Transmit(&hspi2, tx_buf, 2, 50);
    ADXL345_CS_HIGH();
}

uint8_t ADXL345_ReadReg(uint8_t reg) {
    uint8_t tx_buf[2];
    uint8_t rx_buf[2] = {0};

    tx_buf[0] = reg | 0x80; /* 設定 bit7 為 1 (Read 模式) */
    tx_buf[1] = 0x00;       /* Dummy byte，用來產生接收資料的時鐘 */

    ADXL345_CS_LOW();
    /* 使用 TransmitReceive 確保全雙工時序同步 */
    HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 2, 50);
    ADXL345_CS_HIGH();

    return rx_buf[1]; /* 陣列索引 1 為感測器回傳的資料 */
}
void ADXL345_ReadMulti(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t tx_buf[len + 1];
    uint8_t rx_buf[len + 1];

    memset(tx_buf, 0x00, sizeof(tx_buf));
    memset(rx_buf, 0x00, sizeof(rx_buf));

    tx_buf[0] = reg | 0xC0;  /* bit7=1(Read), bit6=1(Multi) */
    /* tx_buf[1..len] 維持 0x00 作為 dummy bytes */

    ADXL345_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, len + 1, DELAY);
    ADXL345_CS_HIGH();

    /* rx_buf[0] 是命令階段的垃圾，真實資料從 index 1 開始 */
    memcpy(buf, &rx_buf[1], len);
}

void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t raw[6];
    // 從 0x32 讀 6 bytes（DATAX0~DATAZ1）
    ADXL345_ReadMulti(0x32, raw, 6);

    *x = (int16_t)((raw[1] << 8) | raw[0]);
    *y = (int16_t)((raw[3] << 8) | raw[2]);
    *z = (int16_t)((raw[5] << 8) | raw[4]);
}
void Test_ADXL345_ID(void) {
    char buf[60] = {0};
    uint8_t id = 0;

    /* 確保 CS 初始為 High */
    ADXL345_CS_HIGH();
    HAL_Delay(10);

    /* 連續讀取 5 次 Device ID (0x00) 驗證穩定性 */
    for (uint8_t i = 1; i <= 5; i++) {
        id = ADXL345_ReadReg(0x00);
        sprintf(buf, "[DEBUG] 4-Wire ADXL345 ID %d/5: 0x%02X\r\n", i, id);
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
        HAL_Delay(100);
    }

    /* 判斷最後一次讀取結果決定是否初始化 */
    if (id == 0xE5) {
        sprintf(buf, "[INFO] ID Correct. Initializing...\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);

        /* DATA_FORMAT: 4-wire SPI (預設), ±2g, FULL_RES */
        ADXL345_WriteReg(0x31, 0x0B);
        /* BW_RATE: 100Hz */
        ADXL345_WriteReg(0x2C, 0x0A);
        /* POWER_CTL: Measure mode */
        ADXL345_WriteReg(0x2D, 0x08);

        sprintf(buf, "[INFO] ADXL345 Init Done.\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
    } else {
        sprintf(buf, "[FATAL] ID Error. Init Aborted.\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
    }
}
void Debug_PrintXYZ(int16_t x, int16_t y, int16_t z) {
    char buf[100] = {0};

    /* 原始值輸出 */
    sprintf(buf, "[RAW]  X=%6d  Y=%6d  Z=%6d\r\n", x, y, z);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), DELAY);

    /* 3.9mg/LSB */
    int32_t gx_01mg = (int32_t)x * 39;
    int32_t gy_01mg = (int32_t)y * 39;
    int32_t gz_01mg = (int32_t)z * 39;

    char sx = (gx_01mg < 0) ? '-' : '+';
    char sy = (gy_01mg < 0) ? '-' : '+';
    char sz = (gz_01mg < 0) ? '-' : '+';

    uint32_t ax = (gx_01mg < 0) ? -gx_01mg : gx_01mg;
    uint32_t ay = (gy_01mg < 0) ? -gy_01mg : gy_01mg;
    uint32_t az = (gz_01mg < 0) ? -gz_01mg : gz_01mg;

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
    sprintf(buf, "[G]    X=%c%lu.%lu mg  Y=%c%lu.%lu mg  Z=%c%lu.%lu mg\r\n",
            sx, ax / 10, ax % 10,
            sy, ay / 10, ay % 10,
            sz, az / 10, az % 10);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), DELAY);
    sprintf(buf, "[G]    X=%c%lu.%02lu g  Y=%c%lu.%02lu g  Z=%c%lu.%02lu g\r\n",
                sx2, ax2 / 100, ax2 % 100,
                sy2, ay2 / 100, ay2 % 100,
                sz2, az2 / 100, az2 % 100);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), DELAY);


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
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  Test_ADXL345_ID();
  /* USER CODE END 2 */
  int16_t ax, ay, az;
  uint32_t count = 0;
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
	  	  ADXL345_ReadXYZ(&ax, &ay, &az);
	  	  Debug_SprintfExample(ax, ay, az);

	  	  Debug_PrintXYZ(ax, ay, az);

	  	  count++;
	  	  if (count % 50 == 0) {
	  	     char buf[40];
	  	     sprintf(buf, "[INFO] %lu samples done\r\n", count);
	  	     HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), DELAY);
	  	  }

	  	     HAL_Delay(10);
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

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
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

/* SPI2 init function */
static void MX_SPI2_Init(void)
{

  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA4 LD2_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
