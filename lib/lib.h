#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "dmesg.h"
#include "pinnames.h"
#include "board.h"

#include "jd_protocol.h"
#include "services/interfaces/jd_oled.h"
#include "services/interfaces/jd_hw_pwr.h"

void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);
void fail_and_reset(void);

#include "tinyhw.h"
#include "dmesg.h"

#define RTC_ALRM_US 10000

// utils.c
int itoa(int n, char *s);
int string_reverse(char *s);
uint32_t random_int(uint32_t max);

#define CHECK(cond)                                                                                \
    if (!(cond))                                                                                   \
        jd_panic()

