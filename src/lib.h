#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "jd_protocol.h"

#include "dmesg.h"
#include "pinnames.h"

#include "services/interfaces/jd_oled.h"
#include "services/interfaces/jd_hw_pwr.h"

void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);

#include "tinyhw.h"
#include "dmesg.h"

#define RTC_ALRM_US 10000

// utils.c
int itoa10(int n, char *s);
int string_reverse(char *s);
uint32_t random_int(uint32_t max);
inline int max(int v1, int v2) {
    return (v1 > v2) ? v1 : v2;
}
inline int min(int v1, int v2) {
    return (v1 < v2) ? v1 : v2;
}

#define CHECK(cond)                                                                                \
    if (!(cond))                                                                                   \
        jd_panic()


typedef struct _queue *jd_queue_t;

jd_queue_t jd_queue_alloc(unsigned size);
int jd_queue_push(jd_queue_t q, jd_frame_t *pkt);
jd_frame_t *jd_queue_front(jd_queue_t q);
void jd_queue_shift(jd_queue_t q);
void jd_queue_test(void);
int jd_queue_will_fit(jd_queue_t q, unsigned size);

void ns_init(void);
void ns_set(uint64_t key, const char *name);
const char *ns_get(uint64_t key);
void ns_clear(void);
