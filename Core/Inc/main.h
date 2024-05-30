/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

#include "stm32g0xx_ll_ucpd.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_dma.h"

#include "stm32g0xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Cell_2S_ADC_Pin GPIO_PIN_2
#define Cell_2S_ADC_GPIO_Port GPIOA
#define Cell_1S_ADC_Pin GPIO_PIN_3
#define Cell_1S_ADC_GPIO_Port GPIOA
#define BAT_ADC_Pin GPIO_PIN_4
#define BAT_ADC_GPIO_Port GPIOA
#define Blue_LED_Pin GPIO_PIN_5
#define Blue_LED_GPIO_Port GPIOA
#define Green_LED_Pin GPIO_PIN_7
#define Green_LED_GPIO_Port GPIOA
#define EN_OTG_Pin GPIO_PIN_0
#define EN_OTG_GPIO_Port GPIOB
#define PROTCHOT_Pin GPIO_PIN_1
#define PROTCHOT_GPIO_Port GPIOB
#define Red_LED_Pin GPIO_PIN_2
#define Red_LED_GPIO_Port GPIOB
#define ILIM_HIZ_Pin GPIO_PIN_11
#define ILIM_HIZ_GPIO_Port GPIOB
#define CHRG_OK_Pin GPIO_PIN_12
#define CHRG_OK_GPIO_Port GPIOB
#define CELL_1S_DIS_EN_Pin GPIO_PIN_4
#define CELL_1S_DIS_EN_GPIO_Port GPIOB
#define CELL_2S_DIS_EN_Pin GPIO_PIN_5
#define CELL_2S_DIS_EN_GPIO_Port GPIOB
#define CELL_3S_DIS_EN_Pin GPIO_PIN_8
#define CELL_3S_DIS_EN_GPIO_Port GPIOB
#define CELL_4S_DIS_EN_Pin GPIO_PIN_9
#define CELL_4S_DIS_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
