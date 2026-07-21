/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum { STATE_SAFE, STATE_WARNING, STATE_DANGER } AttitudeState;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
  //HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

  ssd1306_SetCursor(0,0);

  ssd1306_WriteString("Hei", Font_11x18, White);
  ssd1306_UpdateScreen();
  uint8_t who = 0;
  uint8_t buf[12];
  uint32_t edellinen_aika = HAL_GetTick();

  //Herätä anturi: kirjoita ohjausrekistereihin (tehdään kerran, ennen silmukkaa)
  uint8_t ctrl1_arvo = 0x40;  //104 hz, +-2 g
  uint8_t ctrl2_arvo = 0x40; //104 hz, +- 250 dps

  HAL_I2C_Mem_Write(&hi2c1, 0x6A << 1, 0x10, 1, &ctrl1_arvo, 1, HAL_MAX_DELAY); //CTRL1_XL
  HAL_I2C_Mem_Write(&hi2c1, 0x6A << 1, 0x11, 1, &ctrl2_arvo, 1, HAL_MAX_DELAY); //CTRL2_G


  float alpha = 0.90f;
  float pitch = 0.0f;
  float roll = 0.0f;
  char naytto_teksti[20];
  AttitudeState tila = STATE_SAFE;
  float gy_puskuri [10] = {0};
  int gy_indeksi = 0;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint32_t nyt = HAL_GetTick();
	  float dt = (nyt - edellinen_aika) / 1000.0f;
	  edellinen_aika = nyt;



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
	  //Lue 12 tavua anturidataa kerralla, alkaen tavusta rekisteristä 0x22
	  HAL_I2C_Mem_Read(&hi2c1, 0x6A << 1, 0x22, 1, buf, 12, HAL_MAX_DELAY);

	  //Liimaa tavuparit takaisin 16-bittisiksi lukemiksi
	  int16_t gx = (int16_t) buf[1] << 8 | buf[0];
	  int16_t gy = (int16_t) buf[3] << 8 | buf[2];
	  int16_t gz = (int16_t) buf[5] << 8 | buf[4];
	  int16_t ax = (int16_t) buf[7] << 8 | buf[6];
	  int16_t ay = (int16_t) buf[9] << 8 | buf[8];
	  int16_t az = (int16_t) buf[11] << 8 | buf[10];

	  //muunna gyron raakalukemat asteiksi sekunnissa (dps)
		  float gx_dps = gx * 0.00875f;
		  float gy_dps = gy * 0.00875f;
		  gy_puskuri[gy_indeksi] = gy_dps;
		  gy_indeksi = (gy_indeksi + 1) % 10;

		  float gy_summa = 0.0f;
		  for (int i = 0; i < 10; i++){
			  gy_summa = gy_summa + gy_puskuri[i];
		  }
		  float gy_suodatettu = gy_summa / 10.0f;

		  float gz_dps = gz * 0.00875f;


	  float pitch_acc = atan2f(-ax, sqrtf(ay*ay + az*az)) * 180.0f / M_PI;

	  float roll_acc = atan2f(ay, az) * 180.0f / M_PI;

	  pitch = alpha * (pitch + (gy_dps * dt)) + ((1.0f - alpha) * pitch_acc);
	  roll = alpha * (roll + (gx_dps * dt)) + ((1.0f - alpha ) * roll_acc);

	  float T_lookahead = 1.5f;
	  float pitch_pred = pitch + gy_suodatettu * T_lookahead;
	  printf("Pitch nyt: %.1f  Ennuste: %.1f\r\n", pitch, pitch_pred);
	  if (fabsf(pitch_pred) >= 30.0f){
		  printf("Ennuste: Danger tulossa!\r\n");

	  }


	  printf("acc:%.1f  FILT:%.1f\r\n", pitch_acc, pitch);
	  // ylös: sama raja kuin ennen (kumpi tahansa kulma yli -> ei enää turvassa)
	  if(tila == STATE_SAFE){
		  if (fabsf(pitch) >= 30.0f || fabsf(roll) >= 60.0f){
			  tila = STATE_DANGER;
		  }else if(fabsf(pitch) >= 15.0f || fabsf(roll) >= 30.0f){
			  tila = STATE_WARNING;

		  }
	  }else if(tila == STATE_WARNING){
		  // ylös DANGERiin heti, mutta alas SAFEen vasta selvästi alempana (12 / 25)
		  		  if (fabsf(pitch) >= 30.0f || fabsf(roll) >= 60.0f) {
		  			  tila = STATE_DANGER;
		  		  } else if (fabsf(pitch) < 12.0f && fabsf(roll) < 25.0f) {
		  			  tila = STATE_SAFE;
		  		  }
		  	  } else { // STATE_DANGER
		  		  // alas WARNINGiin vasta selvästi alempana (27 / 55)
		  		  if (fabsf(pitch) < 27.0f && fabsf(roll) < 55.0f) {
		  			  tila = STATE_WARNING;
		  		  }
	  }
	  printf("Tila: %d\r\n", tila);




	 /** if (HAL_I2C_IsDeviceReady(&hi2c1, 0x3C << 1, 3, HAL_MAX_DELAY) == HAL_OK){
		  printf("OLED löytyi (0x3C)! \r\n");
	  } else {
		  printf("OLED ei löytynyt\r\n");
	  }*/

	  ssd1306_Fill(Black);
	  /**
	  sprintf(naytto_teksti, "Pitch:%.1f", pitch);
	  ssd1306_SetCursor(0,0);
	  ssd1306_WriteString(naytto_teksti, Font_11x18, White);
	  sprintf(naytto_teksti, "Roll:%.1f", roll);
	  ssd1306_SetCursor(0,20);
	  ssd1306_WriteString(naytto_teksti, Font_11x18, White);
	  **/

	  int keski_x = 64;
	  int keski_y = 32 + pitch * 1.5f;
	  if (keski_y < 0) keski_y = 0;
	  if (keski_y > 63) keski_y = 63;
	  int puoli_pituus = 40;

	  float kulma_rad = roll * M_PI / 180.0f;
	  int dx = puoli_pituus * cosf(kulma_rad);
	  int dy = puoli_pituus * sinf(kulma_rad);


	  int vasen_y = keski_y - dy;
	  int oikea_y = keski_y + dy;

	  	  if (vasen_y < 0) vasen_y = 0;
	  	  if (vasen_y > 63) vasen_y = 63;
	  	  if (oikea_y < 0) oikea_y = 0;
	  	  if (oikea_y > 63) oikea_y = 63;

	  ssd1306_Line(keski_x - dx, vasen_y, keski_x + dx, oikea_y, White);
	  if(tila == STATE_DANGER){

		  ssd1306_SetCursor(0,46);
		  ssd1306_WriteString("DANGER!!", Font_11x18, White);
		  ssd1306_InvertRectangle(0, 0, 127, 63);

	  }else if (tila == STATE_WARNING){
		 ssd1306_SetCursor(0,46);
		 ssd1306_WriteString("Warning!", Font_11x18, White);
	  }
	  ssd1306_UpdateScreen();

	  HAL_Delay(100);
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

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
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 99;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 416;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 208;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : LD2_Pin PA8 PA9 */
  GPIO_InitStruct.Pin = LD2_Pin|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
	return len;
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
#ifdef USE_FULL_ASSERT
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
