#pragma once

#include "blhw.h"

// exti.c
#define EXTI_FALLING 0x01
#define EXTI_RISING 0x02
void exti_set_callback(uint8_t pin, cb_t callback, uint32_t flags);

// dspi.c
void dspi_init(void);
void dspi_tx(const void *data, uint32_t numbytes, cb_t doneHandler);

void px_init(void);
void px_tx(const void *data, uint32_t numbytes, cb_t doneHandler);
void px_set(const void *data, uint32_t index, uint8_t intensity, uint32_t color);
#define PX_WORDS(NUM_PIXELS) (((NUM_PIXELS)*9 + 8) / 4)

// adc.c
void adc_init_random(void);

// rtc.c
void rtc_init(void);
void rtc_sleep(bool forceShallow);

// pwm.c
uint8_t pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler);
void pwm_set_duty(uint8_t pwm_id, uint32_t duty);
void pwm_enable(uint8_t pwm_id, bool enabled);

// bitbang_spi.c
void bspi_send(const void *src, uint32_t len);
void bspi_recv(void *dst, uint32_t len);

// target_utils.c
void target_reset(void);
uint64_t device_id(void);
RAM_FUNC
void target_wait_cycles(int n);
int target_in_irq(void);

extern uint8_t cpu_mhz;