/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER FUNCTIONS */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

	/******************** PERIPHERAL ENABLE AND CONFIGURATION ********************/
	__HAL_RCC_GPIOA_CLK_ENABLE(); 												// Enable GPIOC clock
	__HAL_RCC_GPIOB_CLK_ENABLE(); 												// Enable GPIOC clock
	__HAL_RCC_GPIOC_CLK_ENABLE(); 												// Enable GPIOC clock
	__HAL_RCC_ADC1_CLK_ENABLE();													// Enable ADC clock
	__HAL_RCC_TIM3_CLK_ENABLE();													// Enable TIM2 clock
	
	// Configure USER button
	GPIO_InitTypeDef initStr_A = {GPIO_PIN_0,
																GPIO_MODE_INPUT,
																GPIO_SPEED_FREQ_LOW,
																GPIO_PULLDOWN};
	HAL_GPIO_Init(GPIOA, &initStr_A); 										// Initialize USER button
																
	// Configure PB1 for analog input
	GPIO_InitTypeDef initStr_B = {GPIO_PIN_1,
																GPIO_MODE_ANALOG};
	HAL_GPIO_Init(GPIOB, &initStr_B); 										// Initialize PB1
	ADC1->CFGR1 	|= 0x2010;															// 8b resoultion, continuous conversion mode, HW triggers disabled
	ADC1->CHSELR 	|= 0x200;																// Channel 9 selected
  
  // Configure LED pins
	GPIO_InitTypeDef initStr_C = {(GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9),
																GPIO_MODE_OUTPUT_PP,
																GPIO_SPEED_FREQ_LOW,
																GPIO_NOPULL};
	HAL_GPIO_Init(GPIOC, &initStr_C); 										// Initialize LEDS pins
																															
	// Timer Configuration
	GPIO_InitTypeDef initStr_C_1 = {GPIO_PIN_6,
																	GPIO_MODE_AF_PP,
																	GPIO_SPEED_FREQ_HIGH,
																	GPIO_PULLDOWN};
	HAL_GPIO_Init(GPIOC, &initStr_C_1);
	GPIOC->AFR[1] |= 0x2;																	// Alternate Function; TIM3_CH1
																		
	TIM3->PSC 		|= 0x18F;																// PSC = 399 for 20kHz timer clock
	TIM3->ARR 		 = 60000-1;															// ARR = 60000 for 3 second max PWM
	TIM3->CCMR1		|= 	TIM_CCMR1_OC1M_2 	| 
										TIM_CCMR1_OC1M_1 	|
										TIM_CCMR1_OC1PE;										// PWM mode 1; Preload enabled
	TIM3->CCER		|= TIM_CCER_CC1E;								
																
	/****************** END PERIPHERAL ENABLE AND CONFIGURATION ******************/
																	
	/****************************** ADC Calibration ******************************/
	/* (1) Ensure that ADEN = 0 */
	/* (2) Clear ADEN by setting ADDIS*/
	/* (3) Clear DMAEN */
	/* (4) Launch the calibration by setting ADCAL */
	/* (5) Wait until ADCAL=0 */
	if ((ADC1->CR & ADC_CR_ADEN) != 0) /* (1) */
	{
		ADC1->CR |= ADC_CR_ADDIS; /* (2) */
	}
	ADC1->CFGR1 &= ~ADC_CFGR1_DMAEN; /* (3) */
	ADC1->CR |= ADC_CR_ADCAL; /* (4) */
	while ((ADC1->CR & ADC_CR_ADCAL) != 0) /* (5) */
	{
		if ((ADC1->CR & ADC_CR_ADCAL) == 0)
		{
			break;
		}
	}
	/**************************** END ADC Calibration ****************************/
	
	/****************************** ADC ENABLE ******************************/
	/* (1) Ensure that ADRDY = 0 */
	/* (2) Clear ADRDY */
	/* (3) Enable the ADC */
	/* (4) Wait until ADC ready */
	if ((ADC1->ISR & ADC_ISR_ADRDY) != 0) /* (1) */
	{
		ADC1->ISR |= ADC_ISR_ADRDY; /* (2) */
	}
	ADC1->CR |= ADC_CR_ADEN; /* (3) */
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0) /* (4) */
	{
		if ((ADC1->ISR & ADC_ISR_ADRDY) == 1)
			break;
	}
	/**************************** END ADC ENABLE ****************************/
	
  /**************************** PRE-LOOP CODE *****************************/
	ADC1->CR |= ADC_CR_ADSTART;														// Start ADC
  uint32_t debouncer = 0;                               // debouncer variable
  uint32_t n = 100;
  uint8_t light_data[100];                              // Light data array
  volatile uint32_t sum;																// Sum for the array data
  volatile uint8_t avg;																	// Avg of light sensor data
	
	uint8_t baseline_li = 128;														// Baseline for light measurement
	uint8_t baseline_delay = 0xFF;												// Baseline speed w.r.t light measurement
	volatile uint16_t shutter_delay;											// Delay used by CCR1 PWM timer
  /*************************** END PRE-LOOP CODE **************************/

  while (1)
  {
    debouncer = (debouncer << 1); 											// Always shift every loop iteration
		if (GPIOA->IDR & 0x1) 
		{ // If input signal is set/high
			debouncer |= 0x01; 																// Set lowest bit of bit-vector
		}
		if (debouncer == 0x7FFFFFFF) 												// if (Push Button)
		{		
			for (int i = 0; i < n; ++i)												// Collect 100 Samples for ligh measurement and sum
      {
        light_data[i] = ADC1->DR;
				sum += light_data[i];
      }
      avg = sum / n;																		// Avg of sum
			sum = 0;
			
			shutter_delay = baseline_li / avg * baseline_delay; // Shutter delay calculation
			
			if (shutter_delay < baseline_delay)								// Set minimum shutter speed 
				shutter_delay = baseline_delay;
			if (shutter_delay > 20000)												// Set maximum shutter speed
				shutter_delay = 20000;*/
			TIM3->CCR1			= shutter_delay - 1;							// Set CCR1 for PWM ie shutter speed
			TIM3->CR1			 |= 0x1;														// Enable counter
			HAL_Delay(1000);																	// Delay for duration of ARR
			TIM3->CR1 &= ~(0x1);															// Disable counter for a maximum of a single pulse
		}
		
  }

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
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

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

