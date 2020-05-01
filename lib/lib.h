#pragma once

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

