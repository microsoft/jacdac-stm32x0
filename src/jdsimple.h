#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "dmesg.h"
#include "pinnames.h"
#include "jdlow.h"
#include "services.h"
#include "host.h"
#include "tinyhw.h"

#define RTC_ALRM_US 10000

// main.c
void led_toggle(void);
void led_set(int state);
void led_blink(int us);
void fail_and_reset(void);

// utils.c
int itoa(int n, char *s);
int string_reverse(char *s);
uint32_t random_int(int max);
void dump_pkt(jd_packet_t *pkt, const char *msg);

// jdapp.c
void app_process(void);
void app_init_services(void);

// txq.c
void txq_init(void);
void txq_flush(void);
int txq_is_idle(void);
void *txq_push(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);

// alloc.c
void alloc_init(void);
void *alloc(uint32_t size);
void alloc_stack_check();
void *alloc_emergency_area(uint32_t size);

// kv.c
void kv_init(void);
void kv_set(uint32_t key, uint32_t val);
bool kv_has(uint32_t key);
uint32_t kv_get(uint32_t key);
uint32_t kv_get_defl(uint32_t key, uint32_t defl);


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
