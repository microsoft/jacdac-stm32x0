#ifndef __STM_H
#define __STM_H

#define STM32G0 1

#include "stm32g0xx_ll_crc.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_exti.h"
#include "stm32g0xx_ll_dma.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_usart.h"
#include "stm32g0xx_ll_tim.h"
#include "stm32g0xx_ll_spi.h"
#include "stm32g0xx_ll_adc.h"
#include "stm32g0xx_ll_rtc.h"
#include "stm32g0xx_ll_i2c.h"

#include "stm32g0xx_hal_rcc.h"

#define HSI_MHZ 16
#define PLL_MHZ 64

// USART2 on lower end G0 can't be set to run from HSI
// we're not currently setup to handle clock freq switching at that time
#if USART_IDX != 1
#define DISABLE_PLL 1
#endif

#endif