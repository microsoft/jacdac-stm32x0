#pragma once

#include "hwconfig.h"

#define RAM_FUNC __attribute__((noinline, long_call, section(".data")))

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

// pins.c
void _pin_set(int pin, int v);
static inline void pin_set(int pin, int v) {
    if ((uint8_t)pin == 0xff)
        return;
    _pin_set(pin, v);
}
void pin_setup_output(int pin);
void pin_toggle(int pin);
int pin_get(int pin);
// pull: -1, 0, 1
void pin_setup_input(int pin, int pull);
void pin_setup_output_af(int pin, int af);
void pin_setup_analog_input(int pin);
void pin_pulse(int pin, int times);

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

// init.c
bool clk_is_pll(void);
void clk_set_pll(int on);


// flash.c
void flash_program(void *dst, const void *src, uint32_t len);
void flash_erase(void *page_addr);

extern uint8_t cpu_mhz;