#ifndef __JDSTM_H
#define __JDSTM_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#if defined(STM32F0)
#include "stm32f0.h"
#elif defined(STM32G0)
#include "stm32g0.h"
#else
#error "invalid CPU"
#endif

#include "jdsimple.h"

extern uint8_t cpu_mhz;

#define exti_disable(pin) LL_EXTI_DisableIT_0_31(pin)
#define exti_enable(pin) LL_EXTI_EnableIT_0_31(pin)
#define exti_enable(pin) LL_EXTI_EnableIT_0_31(pin)
#define exti_clear(pin) LL_EXTI_ClearFallingFlag_0_31(pin)

void tim_set_micros(uint64_t us);
cb_t tim_steal_callback(uint32_t *usec);
void rtc_sync_time(void);
void rtc_cancel_cb(void);

#define PIN_PORT(pin) ((GPIO_TypeDef *)(GPIOA_BASE + (0x400 * (pin >> 4))))
#define PIN_MASK(pin) (1 << ((pin)&0xf))

#ifndef RTC_MIN_TIME_US
#define RTC_MIN_TIME_US 500
#endif

#endif
