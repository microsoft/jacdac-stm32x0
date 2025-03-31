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

/*
 * tim_delay_us - Busy-wait delay for a specified number of microseconds.
 *
 * This function implements a busy-wait loop that delays execution for at least
 * the given number of microseconds. It relies on a 64-bit microsecond timer value,
 * which is obtained from tim_get_micros(). Since the 64-bit timer value is built
 * from a 16-bit hardware counter extended by an offset and may be updated by an
 * interrupt (making its access non-atomic on a 32-bit MCU), the function disables
 * interrupts while reading the timer value to prevent it from being updated during
 * the read operation. This ensures a consistent, atomic snapshot of the 64-bit
 * time value.
 *
 * @param delay_us Number of microseconds to delay.
 */
void tim_delay_us(uint32_t delay_us) {
    uint64_t start_delay_us, now_us;

    // Get the starting time atomically.
    target_disable_irq();
    start_delay_us = tim_get_micros();
    target_enable_irq();

    // Poll until the desired delay has elapsed.
    do {
        // Get the current time atomically.
        target_disable_irq();
        now_us = tim_get_micros();
        target_enable_irq();
    } while ((now_us - start_delay_us) < delay_us);
}

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

    if (delta == JD_MIN_MAX_SLEEP)
        delta = jd_max_sleep;

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
#if defined(STM32G0) || defined(STM32L)
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
    target_disable_irq();
    // first store, the time
    uint64_t us = tim_get_micros();

    // set the prescaler
    LL_TIM_SetPrescaler(TIMx, cpu_mhz - 1);

    // arrange for the update event to be generated very soon to load the new prescaler
    timeoff = us - 0xfffd;
    LL_TIM_DisableCounter(TIMx);
    TIMx->CNT = 0xfffe;
    LL_TIM_EnableCounter(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);

    target_enable_irq();
}
