/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "config.h"
#include "hid.h"
#include "keyboard.h"
#include "ssd1306.h"
#include "ssd1306_conf.h"
#include "ssd1306_fonts.h"
#include "tusb.h"
#include <cdc.h>
#include <ctype.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MOD_WIDTH 38
#define KEY_WIDTH ((SSD1306_WIDTH - MOD_WIDTH) / 3)
#define DIVIDER 32

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
ADC_ChannelConfTypeDef ADC_channel_Config = {0};

extern struct user_config keyboard_user_config;

const uint32_t adc_channels[ADC_CHANNEL_COUNT] = {ADC_CHANNEL_9};
const uint32_t amux_select_pins[AMUX_SELECT_PINS_COUNT] = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_12, GPIO_PIN_13};

extern struct key keyboard_keys[ADC_CHANNEL_COUNT][AMUX_CHANNEL_COUNT];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
  MX_ADC1_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  ADC_channel_Config.Rank = 1;
  ADC_channel_Config.SamplingTime = ADC_SAMPLETIME_3CYCLES;

  keyboard_init_keys();
  ssd1306_Init();
  tusb_rhport_init_t dev_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO};
  tusb_init(0, &dev_init); // initialize device stack on roothub port 0
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    // MARK: Main loop
    tud_task();
    keyboard_task();
    hid_task();
    cdc_task();

    ssd1306_Fill(White);
    ssd1306_FlipScreen(1, 1);

    ssd1306_DrawRectangle(0, 0, SSD1306_WIDTH - 1, SSD1306_HEIGHT - 1, Black);
    ssd1306_Line(MOD_WIDTH, DIVIDER, SSD1306_WIDTH - 1, DIVIDER, Black);
    ssd1306_Line(MOD_WIDTH, 0, MOD_WIDTH, SSD1306_HEIGHT - 1, Black);
    for (int i = 1; i < 3; i++) {
      int x = MOD_WIDTH + i * KEY_WIDTH;
      ssd1306_Line(x, 0, x, SSD1306_HEIGHT - 1, Black);
    }

    int mod_y = 2;
    const int mod_line_height = 10;

    for (int amux = 0; amux < AMUX_CHANNEL_COUNT; amux++) {
      struct key *k = &keyboard_keys[0][amux];

      if (k->state.distance_8bits > 20 && k->layers[_BASE_LAYER].type == KEY_TYPE_MODIFIER) {
        uint8_t bitmask = k->layers[_BASE_LAYER].value;
        const char *label = NULL;

        if (bitmask == 0b00000001)
          label = "LCtrl";
        else if (bitmask == 0b00000010)
          label = "LShift";
        else if (bitmask == 0b00000100)
          label = "LAlt";
        else if (bitmask == 0b00001000)
          label = "LGUI";
        else if (bitmask == 0b00010000)
          label = "RCtrl";
        else if (bitmask == 0b00100000)
          label = "RShift";
        else if (bitmask == 0b01000000)
          label = "RAlt";
        else if (bitmask == 0b10000000)
          label = "RGUI";

        if (label) {
          ssd1306_SetCursor(2, mod_y);
          ssd1306_WriteString(label, Font_6x8, Black);
          mod_y += mod_line_height;
        }
      }
    }

    int label_row_bot = SSD1306_HEIGHT - DIVIDER + 2;
    int percent_row_bot = SSD1306_HEIGHT - 8 - 2;

    int label_row_top = 2;
    int percent_row_top = label_row_bot - 11;

    char keycodes[6][4] = {0};
    uint8_t key_percents[6] = {0};
    int tracker = 0;

    for (int amux = 0; amux < AMUX_CHANNEL_COUNT; amux++) {
      struct key *k = &keyboard_keys[0][amux];

      if (k->state.distance_8bits > 20 && tracker < 6 && k->layers[_BASE_LAYER].type == KEY_TYPE_NORMAL) {
        keycodes[tracker][0] = '0';
        keycodes[tracker][1] = 'x';
        keycodes[tracker][2] = (amux < 10) ? ('0' + amux) : ('A' + (amux - 10));
        keycodes[tracker][3] = '\0';

        key_percents[tracker] = (k->state.distance_8bits * 100) / 255;
        tracker++;
      }
    }

    for (int i = 1; i <= 3; i++) {
      if (keycodes[i - 1][0] != '\0') {
        int x = MOD_WIDTH + (i - 1) * KEY_WIDTH + 4;
        ssd1306_SetCursor(x, label_row_top);
        ssd1306_WriteString((char *)keycodes[i - 1], Font_6x8, Black);

        char buf[6];
        sprintf(buf, "%d%%", key_percents[i - 1]);
        ssd1306_SetCursor(x, percent_row_top);
        ssd1306_WriteString(buf, Font_6x8, Black);
      }
    }

    for (int i = 4; i <= 6; i++) {
      if (keycodes[i - 1][0] != '\0') {
        int x = MOD_WIDTH + (i - 4) * KEY_WIDTH + 4;
        ssd1306_SetCursor(x, label_row_bot);
        ssd1306_WriteString((char *)keycodes[i - 1], Font_6x8, Black);

        char buf[6];
        sprintf(buf, "%d%%", key_percents[i - 1]);
        ssd1306_SetCursor(x, percent_row_bot);
        ssd1306_WriteString(buf, Font_6x8, Black);
      }
    }

    ssd1306_UpdateScreen();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 13;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

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
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
   */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  // memcpy(&ADC_channel_Config, &sConfig, sizeof(ADC_ChannelConfTypeDef));

  /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief USB_OTG_FS Initialization Function
 * @param None
 * @retval None
 */
static void MX_USB_OTG_FS_PCD_Init(void) {

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// MARK: keyboard_* functions

void keyboard_read_config() {
  memcpy(&keyboard_user_config, (uint32_t *)CONFIG_ADDRESS, sizeof(keyboard_user_config));
}

uint8_t keyboard_write_config(uint8_t *buffer, uint16_t offset, uint16_t size) {
  if (offset >= sizeof(keyboard_user_config)) {
    return 0;
  }

  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
  FLASH_Erase_Sector(FLASH_SECTOR_6, VOLTAGE_RANGE_3);
  for (uint16_t i = offset; i < size; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, CONFIG_ADDRESS + i, buffer[i]) != HAL_OK) {
      HAL_FLASH_Lock();
    };
  }
  HAL_FLASH_Lock();
  return 1;
}

void keyboard_select_amux(uint8_t amux_channel) {
  // TODO: set GPIOs at the same time using bitmap on register
  for (uint8_t i = 0; i < AMUX_SELECT_PINS_COUNT; i++) {
    HAL_GPIO_WritePin(GPIOB, amux_select_pins[i], (amux_channel >> i) & 1);
  }
}

void keyboard_select_adc(uint8_t adc_channel) {
  ADC_channel_Config.Channel = adc_channels[adc_channel];
  HAL_ADC_ConfigChannel(&hadc1, &ADC_channel_Config);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 1);
}

uint16_t keyboard_read_adc() {
  return HAL_ADC_GetValue(&hadc1);
}

void keyboard_close_adc() {
  HAL_ADC_Stop(&hadc1);
}

uint32_t keyboard_get_time() {
  return HAL_GetTick();
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
