#include "jdstm.h"

#ifdef SYSTIM_ON_TIM14
#define TIMx TIM14
#define TIMx_IRQn TIM14_IRQn
#define TIMx_IRQHandler TIM14_IRQHandler
#define TIMx_CLK_EN() __HAL_RCC_TIM14_CLK_ENABLE()
#else
#define TIMx TIM17
#define TIMx_IRQn TIM17_IRQn
#define TIMx_IRQHandler TIM17_IRQHandler
#define TIMx_CLK_EN() __HAL_RCC_TIM17_CLK_ENABLE()
#endif

static volatile int64_t timeoff;
static volatile cb_t timer_cb;

uint16_t tim_max_sleep;

uint64_t tim_get_micros(void) {
    while (1) {
        uint32_t v0 = TIMx->CNT;
        uint64_t off = timeoff;
        bool ovr = LL_TIM_IsActiveFlag_UPDATE(TIMx);
        uint32_t v1 = TIMx->CNT;
        if (v0 <= v1) {
            // ovr can be set when the our interrupt is masked
            if (ovr)
                off += 0x10000;
            return off + v1;
        }
    }
}

cb_t tim_steal_callback(uint32_t *usec) {
    cb_t f;
    f = timer_cb;
    if (f) {
        uint16_t delta = LL_TIM_OC_GetCompareCH1(TIMx) - TIMx->CNT;
        *usec = delta;
        if (RTC_MIN_TIME_US <= delta && (int)delta <= (RTC_SECOND_IN_US / 2)) {
            timer_cb = NULL;
        } else {
            f = NULL;
        }
    }
    return f;
}

void tim_set_micros(uint64_t us) {
    timeoff = us;
    LL_TIM_DisableCounter(TIMx);
    TIMx->CNT = 0;
    LL_TIM_EnableCounter(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);
}

void tim_set_timer(int delta, cb_t cb) {
    if (delta < 10)
        delta = 10;

    if (tim_max_sleep && delta == 10000)
        delta = tim_max_sleep;

    rtc_cancel_cb();
    target_disable_irq();
    timer_cb = cb;
    uint16_t nextTrig = TIMx->CNT + (unsigned)delta;
    LL_TIM_OC_SetCompareCH1(TIMx, nextTrig);
    LL_TIM_ClearFlag_CC1(TIMx);
    target_enable_irq();
}

void tim_init(void) {
    /* Peripheral clock enable */
    TIMx_CLK_EN();

    NVIC_SetPriority(TIMx_IRQn, IRQ_PRIORITY_TIM);
    NVIC_EnableIRQ(TIMx_IRQn);

    LL_TIM_SetAutoReload(TIMx, 0xffff);
    LL_TIM_SetPrescaler(TIMx, cpu_mhz - 1);
    LL_TIM_GenerateEvent_UPDATE(TIMx);

    LL_TIM_DisableARRPreload(TIMx);
    LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIMx, LL_TIM_TRGO_RESET);
#if defined(STM32G0) || defined(STM32WL)
    LL_TIM_SetTriggerOutput2(TIMx, LL_TIM_TRGO2_RESET);
#endif
    LL_TIM_DisableMasterSlaveMode(TIMx);

    LL_TIM_ClearFlag_UPDATE(TIMx);
    LL_TIM_EnableIT_UPDATE(TIMx);

    LL_TIM_EnableIT_CC1(TIMx);

    LL_TIM_EnableCounter(TIMx);

    tim_set_timer(5000, NULL);
}

void TIMx_IRQHandler(void) {
    /* Check whether update interrupt is pending */
    if (LL_TIM_IsActiveFlag_UPDATE(TIMx) == 1) {
        /* Clear the update interrupt flag */
        LL_TIM_ClearFlag_UPDATE(TIMx);
        timeoff += 0x10000;
    }

    cb_t f = NULL;

    // need to disable IRQ while checking flags, otherwise we can get IRQ after
    // check, which can in turn set the timer_cb
    target_disable_irq();
    if (LL_TIM_IsActiveFlag_CC1(TIMx) == 1) {
        f = timer_cb;
        timer_cb = NULL;
        LL_TIM_ClearFlag_CC1(TIMx);
    }
    target_enable_irq();

    if (f)
        f();
}

void tim_update_prescaler(void) {
    LL_TIM_DisableCounter(TIMx);
    LL_TIM_SetPrescaler(TIMx, cpu_mhz - 1);
    LL_TIM_EnableCounter(TIMx);
}
