#pragma once

#include "hwconfig.h"

#define RAM_FUNC __attribute__((noinline, long_call, section(".data")))

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

// init.c
bool clk_is_pll(void);
void clk_set_pll(int on);
void clk_setup_pll(void);

// flash.c
void flash_program(void *dst, const void *src, uint32_t len);
void flash_erase(void *page_addr);

void jd_panic(void);
void target_reset(void);
void target_wait_us(uint32_t n);
