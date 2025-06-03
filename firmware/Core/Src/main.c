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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
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
ADC_HandleTypeDef hadc1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define S0_PIN GPIO_PIN_9
#define S1_PIN GPIO_PIN_8
#define S2_PIN GPIO_PIN_7
#define S3_PIN GPIO_PIN_6
#define S_GPIO_PORT GPIOB
char uart_buf[64];
extern USBD_HandleTypeDef hUsbDeviceFS;
void set_mux_channel(uint8_t channel) {
    HAL_GPIO_WritePin(S_GPIO_PORT, S0_PIN, (channel & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_GPIO_PORT, S1_PIN, (channel & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_GPIO_PORT, S2_PIN, (channel & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_GPIO_PORT, S3_PIN, (channel & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_Delay(1); // Short delay for settling
}

// Function to read ADC value
uint32_t read_adc_value() {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    return HAL_ADC_GetValue(&hadc1);
}
typedef struct
{
	uint8_t MODIFIER;
	uint8_t RESERVED;
	uint8_t KEYCODE1;
	uint8_t KEYCODE2;
	uint8_t KEYCODE3;
	uint8_t KEYCODE4;
	uint8_t KEYCODE5;
	uint8_t KEYCODE6;
}subKeyBoard;

subKeyBoard keyBoardHIDsub = {0,0,0,0,0,0,0,0};

//void send_usb_message(void)
//{
//    char msg[] = "Hello from STM32 via Virtual COM\r\n";
//    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
//}
void typeOnKeyboard(char cmd[]){

	for(int i = 0; i < strlen(cmd); i++) {
		char charToType = cmd[i];
		uint8_t KEYCODE = 0x00;
		uint8_t MODIFIER = 0x00;

		switch(charToType) {
			case 'A': KEYCODE = 0x04; MODIFIER=0x02; break;
		    case 'B': KEYCODE = 0x05; MODIFIER=0x02; break;
		    case 'C': KEYCODE = 0x06; MODIFIER=0x02; break;
		    case 'D': KEYCODE = 0x07; MODIFIER=0x02; break;
		    case 'E': KEYCODE = 0x08; MODIFIER=0x02; break;
		    case 'F': KEYCODE = 0x09; MODIFIER=0x02; break;
		    case 'G': KEYCODE = 0x0A; MODIFIER=0x02; break;
		    case 'H': KEYCODE = 0x0B; MODIFIER=0x02; break;
		    case 'I': KEYCODE = 0x0C; MODIFIER=0x02; break;
		    case 'J': KEYCODE = 0x0D; MODIFIER=0x02; break;
		    case 'K': KEYCODE = 0x0E; MODIFIER=0x02; break;
		    case 'L': KEYCODE = 0x0F; MODIFIER=0x02; break;
		    case 'M': KEYCODE = 0x10; MODIFIER=0x02; break;
		    case 'N': KEYCODE = 0x11; MODIFIER=0x02; break;
		    case 'O': KEYCODE = 0x12; MODIFIER=0x02; break;
		    case 'P': KEYCODE = 0x13; MODIFIER=0x02; break;
		    case 'Q': KEYCODE = 0x14; MODIFIER=0x02; break;
		    case 'R': KEYCODE = 0x15; MODIFIER=0x02; break;
		    case 'S': KEYCODE = 0x16; MODIFIER=0x02; break;
		    case 'T': KEYCODE = 0x17; MODIFIER=0x02; break;
		    case 'U': KEYCODE = 0x18; MODIFIER=0x02; break;
		    case 'V': KEYCODE = 0x19; MODIFIER=0x02; break;
		    case 'W': KEYCODE = 0x1A; MODIFIER=0x02; break;
		    case 'X': KEYCODE = 0x1B; MODIFIER=0x02; break;
		    case 'Y': KEYCODE = 0x1C; MODIFIER=0x02; break;
		    case 'Z': KEYCODE = 0x1D; MODIFIER=0x02; break;
		    case 'a': KEYCODE = 0x04; break;
		    case 'b': KEYCODE = 0x05; break;
		    case 'c': KEYCODE = 0x06; break;
		    case 'd': KEYCODE = 0x07; break;
		    case 'e': KEYCODE = 0x08; break;
		    case 'f': KEYCODE = 0x09; break;
		    case 'g': KEYCODE = 0x0A; break;
		    case 'h': KEYCODE = 0x0B; break;
		    case 'i': KEYCODE = 0x0C; break;
		    case 'j': KEYCODE = 0x0D; break;
		    case 'k': KEYCODE = 0x0E; break;
		    case 'l': KEYCODE = 0x0F; break;
		    case 'm': KEYCODE = 0x10; break;
		    case 'n': KEYCODE = 0x11; break;
		    case 'o': KEYCODE = 0x12; break;
		    case 'p': KEYCODE = 0x13; break;
		    case 'q': KEYCODE = 0x14; break;
		    case 'r': KEYCODE = 0x15; break;
		    case 's': KEYCODE = 0x16; break;
		    case 't': KEYCODE = 0x17; break;
		    case 'u': KEYCODE = 0x18; break;
		    case 'v': KEYCODE = 0x19; break;
		    case 'w': KEYCODE = 0x1A; break;
		    case 'x': KEYCODE = 0x1B; break;
		    case 'y': KEYCODE = 0x1C; break;
		    case 'z': KEYCODE = 0x1D; break;
		    case '1': KEYCODE = 0x1E; break;
		    case '2': KEYCODE = 0x1F; break;
		    case '3': KEYCODE = 0x20; break;
		    case '4': KEYCODE = 0x21; break;
		    case '5': KEYCODE = 0x22; break;
		    case '6': KEYCODE = 0x23; break;
		    case '7': KEYCODE = 0x24; break;
		    case '8': KEYCODE = 0x25; break;
		    case '9': KEYCODE = 0x26; break;
		    case '0': KEYCODE = 0x27; break;
		    case ' ': KEYCODE = 0x2C; break;
		    case '-': KEYCODE = 0x2D; break;
		    case ';': KEYCODE = 0x33; break; // semi-colon
		    case ':': KEYCODE = 0x33; MODIFIER=0x02; break; // colon
		    case '\'': KEYCODE = 0x34; break; // single quote
		    case '\"': KEYCODE = 0x34; MODIFIER=0x02; break; // double quote
		    case '/': KEYCODE = 0x38; break; // slash
		    case '.': KEYCODE = 0x37; break; // slash
		    default: KEYCODE = 0x00; break; // Default case for undefined characters
		}


		 keyBoardHIDsub.MODIFIER=MODIFIER;  // To press shift key
		 keyBoardHIDsub.KEYCODE1=KEYCODE;

		 USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));


		 keyBoardHIDsub.MODIFIER=0x00;
		 keyBoardHIDsub.KEYCODE1=0x00;

		 USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));

	}

}

void keyPressRelease(uint8_t MODIFIER, uint8_t KEYCODE){

	 keyBoardHIDsub.KEYCODE1=KEYCODE;

	 USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));
	 HAL_Delay(15);
	 keyBoardHIDsub.KEYCODE1=0x00;
	 USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));




}


// Format: "2345,3456,...,1023\n"
void send_adc_data_frame(void) {
//    char uart_buf[128];
//    char *p = uart_buf;
    for (uint8_t i = 0; i < 16; i++) {
        set_mux_channel(i);
        uint32_t value = read_adc_value();
        if ((i==2) && (value < 2045)){

        	keyPressRelease(0x00,0x04);
        }
        if ((i==8) && (value < 1900)){

        	keyPressRelease(0x00,0x05);
        }

//        p += sprintf(p, "%lu,", value);
    }
//    *(p - 1) = '\n';  // replace last comma with newline
//    CDC_Transmit_FS((uint8_t*)uart_buf, strlen(uart_buf));
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
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//	  for (uint8_t i = 0; i < 16; i++) {
//	              set_mux_channel(i);
//	              uint32_t value = read_adc_value();
//	              snprintf(uart_buf, sizeof(uart_buf), "Channel %2d: %4lu\r\n", i, value);
//	              CDC_Transmit_FS((uint8_t*)uart_buf, strlen(uart_buf));
//
//	          }
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

	  send_adc_data_frame();
	  HAL_Delay(1);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15
                           PB6 PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
