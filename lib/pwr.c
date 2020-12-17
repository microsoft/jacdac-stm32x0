#include "lib.h"

static uint8_t pll_cnt, tim_cnt, no_sleep_cnt;

bool pwr_in_pll() {
    return pll_cnt > 0;
}

static void check_overflow(uint8_t v) {
    if (target_in_irq() || v == 0xff)
        jd_panic(); // should not be called in ISR handler
}

void pwr_enter_pll() {
    check_overflow(pll_cnt);
    if (pll_cnt == 0)
        clk_set_pll(1);
    pll_cnt++;
}

void pwr_leave_pll() {
    if (!pll_cnt)
        jd_panic();
    pll_cnt--;
    if (pll_cnt == 0)
        clk_set_pll(0);
}

void pwr_enter_tim() {
    check_overflow(tim_cnt);
    tim_cnt++;
}

void pwr_leave_tim() {
    if (!tim_cnt)
        jd_panic();
    tim_cnt--;
}

void pwr_wait_tim() {
    while (pll_cnt || tim_cnt)
        rtc_sleep(true);
}

void pwr_enter_no_sleep() {
    check_overflow(no_sleep_cnt);
    no_sleep_cnt++;
}

void pwr_leave_no_sleep() {
    if (!no_sleep_cnt)
        jd_panic();
    no_sleep_cnt--;
}

void pwr_sleep() {
    if (no_sleep_cnt)
        return;
    rtc_sleep(pll_cnt || tim_cnt || jd_is_busy());
}
