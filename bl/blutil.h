#pragma once

#include "jdprofile.h"
#include "blproto.h"

#if defined(STM32F0)
#include "stm32f0.h"
#elif defined(STM32G0)
#include "stm32g0.h"
#else
#error "invalid CPU"
#endif

#include "jd_physical.h"
#include "jd_control.h"

#ifndef QUICK_LOG
#define QUICK_LOG 0
#endif

#define CPU_MHZ HSI_MHZ

void blled_init(uint32_t period);
void blled_set_duty(uint32_t duty);

void led_init(void);
void led_set_value(int v);
void hw_panic(void);

uint16_t crc16(const void *data, uint32_t size);
