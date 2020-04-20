#include "bl.h"
#define TIMx TIM17

static uint32_t timeoff;

static void tim_update(void) {
    if (LL_TIM_IsActiveFlag_UPDATE(TIMx) == 1) {
        LL_TIM_ClearFlag_UPDATE(TIMx);
        timeoff += 0x10000;
        // reset when the timer reaches 2b us (half hour);
        // this is more than enough for bootloader and avoids problems with overflows
        if (timeoff >> 31)
            target_reset();
    }
}

uint32_t tim_get_micros() {
    while (1) {
        uint32_t v0 = TIMx->CNT;
        tim_update();
        uint32_t v1 = TIMx->CNT;
        if (v0 <= v1)
            return timeoff + v1;
    }
}

void tim_init() {
    LL_TIM_SetAutoReload(TIMx, 0xffff);
    LL_TIM_SetPrescaler(TIMx, CPU_MHZ - 1);
    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);
    LL_TIM_EnableCounter(TIMx);
}
