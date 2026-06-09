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

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define ADXL345_CS_PORT GPIOB
#define ADXL345_CS_PIN  GPIO_PIN_12
#define ADXL345_CS_LOW()  HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_RESET)
#define ADXL345_CS_HIGH() HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_SET)

#define TIM_CLK_LOW() HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET);
#define TIM_CLK_HIGH() HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);

#define TIM_DIO_LOW() HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET);
#define TIM_DIO_HIGH() HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET);

#define DELAY 50



/* TM1637 */
#define TM_GND_GPIO_Port  GPIOA
#define TM_GND_Pin        GPIO_PIN_0
#define TM_VCC_GPIO_Port  GPIOA
#define TM_VCC_Pin        GPIO_PIN_1

#define TM_DIO_GPIO_Port  GPIOA
#define TM_DIO_Pin        GPIO_PIN_4
#define TM_CLK_GPIO_Port  GPIOB
#define TM_CLK_Pin        GPIO_PIN_0
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
                                    
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                                

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

// === Senor == //
void Debug_SprintfExample(int16_t x, int16_t y, int16_t z) {
    char buf[50];
    sprintf(buf, "[DATA] raw X=%6d Y=%6d Z=%6d\r\n", x, y, z);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf),  DELAY);

}
void ADXL345_WriteReg(uint8_t reg, uint8_t data) {
    uint8_t tx_buf[2];
    tx_buf[0] = reg & 0x3F; /* 確保 bit7(Read) 為 0, bit6(MB) 為 0 */
    tx_buf[1] = data;

    ADXL345_CS_LOW();
    HAL_SPI_Transmit(&hspi2, tx_buf, 2, DELAY);
    ADXL345_CS_HIGH();
}
uint8_t ADXL345_ReadReg(uint8_t reg) {
    uint8_t tx_buf[2];
    uint8_t rx_buf[2] = {0};

    tx_buf[0] = reg | 0x80; /* 設定 bit7 為 1 (Read 模式) */
    tx_buf[1] = 0x00;       /* Dummy byte，用來產生接收資料的時鐘 */
 	ADXL345_CS_LOW();
 	HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 2, 50);
    ADXL345_CS_HIGH();
    return rx_buf[1]; /* 陣列索引 1 為感測器回傳的資料，0是dummy 等待時的垃圾值 */

}
void ADXL345_ReadMulti(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t tx_buf[len + 1];
    uint8_t rx_buf[len + 1];

    memset(tx_buf, 0x00, sizeof(tx_buf));
    memset(rx_buf, 0x00, sizeof(rx_buf));

    tx_buf[0] = reg | 0xC0;

    ADXL345_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, len + 1, DELAY);
    ADXL345_CS_HIGH();

    memcpy(buf, &rx_buf[1], len);  /* index 0 捨棄 */
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

        /* DATA_FORMAT: 4-wire SPI (預設), */
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

//    /* 3.9mg/LSB */
//    int32_t gx_01mg = (int32_t)x * 39;
//    int32_t gy_01mg = (int32_t)y * 39;
//    int32_t gz_01mg = (int32_t)z * 39;
//
//    char sx = (gx_01mg < 0) ? '-' : '+';
//    char sy = (gy_01mg < 0) ? '-' : '+';
//    char sz = (gz_01mg < 0) ? '-' : '+';
//
//    uint32_t ax = (gx_01mg < 0) ? -gx_01mg : gx_01mg;
//    uint32_t ay = (gy_01mg < 0) ? -gy_01mg : gy_01mg;
//    uint32_t az = (gz_01mg < 0) ? -gz_01mg : gz_01mg;

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
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), DELAY);


}
// =========== //

// ====Display === //
// 延時讓波形更穩定
//void TM1637_Delay(void) {
//	for(volatile int i=0; i<400; i++);
//}
//void TM1637_Start(void) {
//	TIM_CLK_HIGH();
//	TIM_DIO_HIGH();
//    TM1637_Delay();
//	TIM_DIO_LOW();
//}
//void TM1637_Stop(void) {
//	TIM_CLK_LOW()
//    TM1637_Delay();
//	TIM_DIO_LOW();
//    TM1637_Delay();
//
//	TIM_CLK_HIGH();
//    TM1637_Delay();
//    TIM_DIO_HIGH();
//}
//
//uint8_t TM1637_WriteByte(uint8_t byte) {
//	uint8_t ack = 0;
//    for (uint8_t i = 0; i < 8; i++) {
//    	TIM_CLK_LOW();
//        TM1637_Delay();
//
//        /* LSB First 寫入 */
//        if (byte & (1 << i)) {
//        	TIM_DIO_HIGH();
//        } else {
//        	TIM_DIO_LOW();
//        }
//
//        TM1637_Delay();
//        TIM_CLK_HIGH();
//        TM1637_Delay();
//    }
//
//    /* 產生第 9 個 Clock 並釋放 DIO 等待 ACK */
//    TIM_CLK_LOW();
//    TIM_DIO_HIGH();
//    TM1637_Delay();
//    TIM_CLK_HIGH();
//    TM1637_Delay();
//
//    /* 讀取 DIO 狀態：如果被 TM1637 拉低，代表有 ACK */
//    if (HAL_GPIO_ReadPin(TM_DIO_GPIO_Port, TM_DIO_Pin) == GPIO_PIN_RESET) {
//        ack = 1; // 成功收到 ACK
//    	char msg[] = "ack success";
//    	HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
//    } else {
//        ack = 0; // 失敗，沒有 ACK
//        char msg[] = "ack failed";
//        HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
//    }
//
//    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET); // 完成第 9 個 Clock
//    TM1637_Delay();
//
//    return ack;
//}
//void TM1637_DisplayInt_Debug(int value) {
//    const uint8_t digitMap[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
//    uint8_t data[4] = {0, 0, 0, 0};
//    uint8_t ack_cmd1 = 0, ack_cmd2 = 0, ack_cmd3 = 0;
//    char dbg_buf[120];
//
//    /* 數值硬體極限保護 */
//    if (value > 9999) value = 9999;
//    if (value < -999) value = -999;
//
//    int temp = (value < 0) ? -value : value;
//
//    /* 轉換數字 */
//    for(int i = 3; i >= 0; i--) {
//        data[i] = digitMap[temp % 10];
//        temp /= 10;
//        if(temp == 0) {
//        	char msg[] = "break in transfer\r\n";
//        	HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
//        	break;
//        }
//    }
//
//    /* 動態負號處理 */
//    if (value < 0) {
//        for(int i = 2; i >= 0; i--) {
//            if (data[i] == 0) {
//                data[i] = 0x40; // 0x40 為中間橫線 '-'
//                char msg[] = "break in dynamic transfer\r\n";
//                HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
//
//                break;
//            }
//        }
//    }
//
//    /* 發送資料 */
//    TM1637_Start();
//    ack_cmd1 = TM1637_WriteByte(0x40);
//    TM1637_WriteByte(0x40); // 自動地址加一
//    TM1637_Stop();
//
//    TM1637_Start();
//    TM1637_WriteByte(0xC0); // 起始地址 0x00
//    for(int i = 0; i < 4; i++) {
//        TM1637_WriteByte(data[i]);
//    }
//    TM1637_Stop();
//
//    TM1637_Start();
//    TM1637_WriteByte(0x88); // 開啟顯示，亮度設為最低以保護 GPIO
//    TM1637_Stop();
//
//    sprintf(dbg_buf, "[TM] Val:%5d | Codes: %02X %02X %02X %02X | ACK(1=OK): %d,%d,%d\r\n",
//                value, data[0], data[1], data[2], data[3], ack_cmd1, ack_cmd2, ack_cmd3);
//    HAL_UART_Transmit(&huart2, (uint8_t *)dbg_buf, strlen(dbg_buf), 100);
//}
//
//void Debug_CheckPowerPins(void) {
//    char buf[100];
//
//    // 讀取 PA0 和 PA1 的輸出狀態 (ODR 暫存器)
//    GPIO_PinState state_PA0 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
//    GPIO_PinState state_PA1 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
//    GPIO_PinState state_PA4 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
//
//    // 轉換為容易閱讀的文字
//    const char* str_PA0 = (state_PA0 == GPIO_PIN_SET) ? "HIGH (3.3V)" : "LOW (0V)";
//    const char* str_PA1 = (state_PA1 == GPIO_PIN_SET) ? "HIGH (3.3V)" : "LOW (0V)";
//    const char* str_PA4 = (state_PA4 == GPIO_PIN_SET) ? "[DIO] HIGH (3.3V)" : "[DIO] LOW (0V)";
//
//    sprintf(buf, "[DEBUG] PA0(GND) state: %s | PA1(VCC) state: %s | PA4(DIO) state: %s \r\n", str_PA0, str_PA1,str_PA4);
//    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 100);
//}




void TM1637_Delay(void) {
    /* 增加延遲常數，避免高系統頻率導致時序過短 */
    for(volatile int i=0; i<400; i++);
}

void TM1637_Start(void) {
    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET);
}

void TM1637_Stop(void) {
    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
}

void TM1637_WriteByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET);
        TM1637_Delay();

        /* LSB First 寫入 */
        if (byte & (1 << i)) {
            HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_RESET);
        }

        TM1637_Delay();
        HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
        TM1637_Delay();
    }

    /* 產生第 9 個 Clock 並釋放 DIO 等待 ACK */
    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TM_DIO_GPIO_Port, TM_DIO_Pin, GPIO_PIN_SET);
    TM1637_Delay();
    HAL_GPIO_WritePin(TM_CLK_GPIO_Port, TM_CLK_Pin, GPIO_PIN_SET);
    TM1637_Delay();
}

void TM1637_DisplayInt(int value) {
    const uint8_t digitMap[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
    uint8_t data[4] = {0, 0, 0, 0};

    /* 數值硬體極限保護 */
    if (value > 9999) value = 9999;
    if (value < -999) value = -999;

    int temp = (value < 0) ? -value : value;

    /* 轉換數字 */
    for(int i = 3; i >= 0; i--) {
        data[i] = digitMap[temp % 10];
        temp /= 10;
        if(temp == 0) break;
    }

    /* 動態負號處理 */
    if (value < 0) {
        for(int i = 2; i >= 0; i--) {
            if (data[i] == 0) {
                data[i] = 0x40; // 0x40 為中間橫線 '-'
                break;
            }
        }
    }

    /* 發送資料 */
    TM1637_Start();
    TM1637_WriteByte(0x40); // 自動地址加一
    TM1637_Stop();

    TM1637_Start();
    TM1637_WriteByte(0xC0); // 起始地址 0x00
    for(int i = 0; i < 4; i++) {
        TM1637_WriteByte(data[i]);
    }
    TM1637_Stop();

    TM1637_Start();
    TM1637_WriteByte(0x88); // 開啟顯示，亮度設為最低以保護 GPIO
    TM1637_Stop();
}

void TM1637_DisplayG(int16_t raw_accel) {
    const uint8_t digitMap[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
    uint8_t data[4] = {0, 0, 0, 0};

    /* 1 LSB = 3.9 mg。計算 g * 100 的整數值，用以表示小數點後兩位 */
    int32_t val = ((int32_t)raw_accel * 39) / 100;

    /* 極限保護：四位數顯示器最多顯示到 99.99 或 -9.99，此處限制為常見的 +-16g 範圍 */
    if (val > 1600) val = 1600;
    if (val < -999) val = -999;

    int temp = (val < 0) ? -val : val;

    /* 固定填入小數點後兩位與個位數 */
    data[3] = digitMap[temp % 10];         // 小數點後第 2 位
    temp /= 10;
    data[2] = digitMap[temp % 10];         // 小數點後第 1 位
    temp /= 10;
    data[1] = digitMap[temp % 10] | 0x80;  // 個位數，並加上小數點 (0x80)

    /* 處理符號或十位數 */
    if (val < 0) {
        data[0] = 0x40; // 顯示負號 '-'
    } else {
        temp /= 10;
        if (temp > 0) {
            data[0] = digitMap[temp % 10]; // 若數值大於等於 10g，顯示十位數
        } else {
            data[0] = 0x00; // 否則最高位全滅 (空白)
        }
    }

    /* 發送資料 */
    TM1637_Start();
    TM1637_WriteByte(0x40); // 自動地址加一
    TM1637_Stop();

    TM1637_Start();
    TM1637_WriteByte(0xC0); // 起始地址 0x00
    for(int i = 0; i < 4; i++) {
        TM1637_WriteByte(data[i]);
    }
    TM1637_Stop();

    TM1637_Start();
    TM1637_WriteByte(0x88); // 開啟顯示，亮度最低
    TM1637_Stop();
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
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(TM_GND_GPIO_Port, TM_GND_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TM_VCC_GPIO_Port, TM_VCC_Pin, GPIO_PIN_SET);
  HAL_Delay(50); // 等待供電穩定

  /* 初始化 ADXL345 */
  Test_ADXL345_ID();

  // Debug_CheckPowerPins();


//  int axisMode = 0; // 0: 顯示X, 1: 顯示Y, 2: 顯示Z
//  uint8_t lastBtnState = GPIO_PIN_SET; // 假設按鍵接 PC13，預設被 Pull-up
  int16_t ax, ay, az;

  int axisMode = 0; // 0: 顯示X, 1: 顯示Y, 2: 顯示Z
  uint8_t lastBtnState = GPIO_PIN_SET; // 假設按鍵接 PC13，預設被 Pull-up
  //uint32_t count = 0;
  // Test_ADXL345_ID();
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */
  /* USER CODE BEGIN 3 */
//	  uint8_t currentBtnState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
//	  if (lastBtnState == GPIO_PIN_SET && currentBtnState == GPIO_PIN_RESET) {
//	     axisMode++;
//	     if (axisMode > 2) {
//	        axisMode = 0;
//	     }
//	        HAL_Delay(50); // 簡易去彈跳
//	     }
//	     lastBtnState = currentBtnState;

//	  ADXL345_ReadXYZ(&ax, &ay, &az);
//	  Debug_PrintXYZ(ax, ay, az);
//	  if (axisMode == 0) {
//	      TM1637_DisplayInt_Debug(ax);
//	  } else if (axisMode == 1) {
//	      TM1637_DisplayInt_Debug(ay);
//	  } else if (axisMode == 2) {
//	      TM1637_DisplayInt_Debug(az);
//	 }
// 	  HAL_Delay(100);
	  uint8_t currentBtnState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
	          if (lastBtnState == GPIO_PIN_SET && currentBtnState == GPIO_PIN_RESET) {
	              axisMode++;
	              if (axisMode > 2) {
	                  axisMode = 0;
	              }
	              HAL_Delay(50); // 簡易去彈跳
	          }
	          lastBtnState = currentBtnState;

	          /* 讀取感測器資料 */
	          ADXL345_ReadXYZ(&ax, &ay, &az);

	          /* UART 輸出除錯資訊 */
	          Debug_PrintXYZ(ax, ay, az);

	          /* 依據 axisMode 顯示對應軸向的raw 數值到 TM1637 */
//			  if (axisMode == 0) {
//			     TM1637_DisplayInt_Debug(ax);
//			  } else if (axisMode == 1) {
//			     TM1637_DisplayInt_Debug(ay);
//			  } else if (axisMode == 2) {
//			     TM1637_DisplayInt_Debug(az);
//			  }

	          /* 依據 axisMode 顯示對應軸向的數值到 TM1637 */
	         if (axisMode == 0) {
	            TM1637_DisplayG(ax);
	         } else if (axisMode == 1) {
	            TM1637_DisplayG(ay);
	         } else if (axisMode == 2) {
	            TM1637_DisplayG(az);
	         }
	          HAL_Delay(100);
  /* USER CODE END 3 */
}
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

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
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

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim1);

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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 LD2_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
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
