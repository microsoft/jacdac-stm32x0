#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "dmesg.h"
#include "pinnames.h"
#include "board.h"

#include "jd_protocol.h"

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
// do WFI until PLL/TIM mode is left
void pwr_wait_tim(void);

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