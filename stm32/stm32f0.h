#ifndef __STM_H
#define __STM_H

#define STM32F0 1

#include "board.h"

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
#include "stm32f0xx_ll_i2c.h"

#include "stm32f0xx_hal_rcc.h"

#define LL_EXTI_ClearFallingFlag_0_31 LL_EXTI_ClearFlag_0_31
#define LL_EXTI_IsActiveFallingFlag_0_31 LL_EXTI_IsActiveFlag_0_31
#define __HAL_RCC_ADC_CLK_ENABLE __HAL_RCC_ADC1_CLK_ENABLE
#define __HAL_RCC_ADC_CLK_DISABLE __HAL_RCC_ADC1_CLK_DISABLE

#define HSI_MHZ 8
#define PLL_MHZ 48

// USART2 on lower end F0 can't be set to run from HSI
// we're not currently setup to handle clock freq switching at that time
#if USART_IDX != 1 || defined(ONE_WIRE)
#define DISABLE_PLL 1
#endif

#endif
