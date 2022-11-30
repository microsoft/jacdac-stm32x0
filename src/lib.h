#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "jd_protocol.h"

#include "dmesg.h"
#include "pinnames.h"

#include "services/interfaces/jd_oled.h"
#include "services/interfaces/jd_hw_pwr.h"
#include "interfaces/jd_usb.h"

void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);

#include "tinyhw.h"
#include "dmesg.h"

#define RTC_ALRM_US 10000

// utils.c
uint32_t random_int(uint32_t max);
inline int max(int v1, int v2) {
    return (v1 > v2) ? v1 : v2;
}
inline int min(int v1, int v2) {
    return (v1 < v2) ? v1 : v2;
}

#define CHECK(cond)                                                                                \
    if (!(cond))                                                                                   \
        JD_PANIC()


void ns_init(void);
void ns_set(uint64_t key, const char *name);
const char *ns_get(uint64_t key);
void ns_clear(void);
