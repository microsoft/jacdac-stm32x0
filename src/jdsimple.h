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
void txq_flush(void);
int txq_is_idle(void);
void *txq_push(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);

// alloc.c
void alloc_init(void);
void *alloc(uint32_t size);
void alloc_stack_check();

// pwr.c
void pwr_enter_pll(void);
void pwr_leave_pll(void);
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
