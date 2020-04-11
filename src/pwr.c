#include "jdsimple.h"

static uint8_t pll_cnt;

bool pwr_in_pll() {
    return pll_cnt > 0;
}

void pwr_enter_pll() {
    if (target_in_irq())
        jd_panic(); // should not be called in ISR handler
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

void pwr_sleep() {
    rtc_sleep(pll_cnt || jd_is_busy());
}
