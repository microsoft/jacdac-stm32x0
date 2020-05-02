#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "dmesg.h"
#include "pinnames.h"
#include "board.h"

typedef void (*cb_t)(void);

// Required by jdlow.c
void jd_panic(void);
void target_enable_irq(void);
void target_disable_irq(void);
void target_wait_us(uint32_t n);

void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);

// Provided jdutil.c
uint32_t jd_random_around(uint32_t v);
uint32_t jd_random(void);
void jd_seed_random(uint32_t s);
uint32_t jd_hash_fnv1a(const void *data, unsigned len);
uint16_t jd_crc16(const void *data, uint32_t size);

int jd_is_busy(void);
void fail_and_reset(void);

#include "tinyhw.h"

#define RTC_ALRM_US 10000

// utils.c
int itoa(int n, char *s);
int string_reverse(char *s);
uint32_t random_int(uint32_t max);

// alloc.c
void alloc_init(void);
void *alloc(uint32_t size);
void alloc_stack_check(void);
void *alloc_emergency_area(uint32_t size);

// pwr.c
// enter/leave high-speed no-deep-sleep mode
void pwr_enter_pll(void);
void pwr_leave_pll(void);
bool pwr_in_pll(void);
// enter/leave no-deep-sleep mode
void pwr_enter_tim(void);
void pwr_leave_tim(void);
// go to sleep, deep or shallow
void pwr_sleep(void);

extern uint32_t now;

// check if given timestamp is already in the past, regardless of overflows on 'now'
// the moment has to be no more than ~500 seconds in the past
static inline bool in_past(uint32_t moment) {
    return ((now - moment) >> 29) == 0;
}
static inline bool in_future(uint32_t moment) {
    return ((moment - now) >> 29) == 0;
}

// keep sampling at period, using state at *sample
bool should_sample(uint32_t *sample, uint32_t period);

#define CHECK(cond)                                                                                \
    if (!(cond))                                                                                   \
    jd_panic()


uint32_t hw_temp(void);
uint32_t hw_humidity(void);

#define OLED_WIDTH 64
#define OLED_HEIGHT 48

typedef struct oled_state {
    uint8_t databuf[OLED_WIDTH * OLED_HEIGHT / 8];
} oled_state_t;

void oled_set_pixel(oled_state_t *ctx, int x, int y);
void oled_setup(oled_state_t *ctx);
void oled_flush(oled_state_t *ctx);

