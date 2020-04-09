#ifndef __STM_H
#define __STM_H

#define STM32F0 1

#include "stm32f0xx_ll_crc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_usart.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_adc.h"
#include "stm32f0xx_ll_rtc.h"

#include "stm32f0xx_hal_rcc.h"

#define LL_EXTI_ClearFallingFlag_0_31 LL_EXTI_ClearFlag_0_31
#define LL_EXTI_IsActiveFallingFlag_0_31 LL_EXTI_IsActiveFlag_0_31
#define __HAL_RCC_ADC_CLK_ENABLE __HAL_RCC_ADC1_CLK_ENABLE
#define __HAL_RCC_ADC_CLK_DISABLE __HAL_RCC_ADC1_CLK_DISABLE

#ifdef LOW_POWER
#define CPU_MHZ 8
#else
#define CPU_MHZ 48
#endif

#endif