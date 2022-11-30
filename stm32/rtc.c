#include "jdstm.h"

#define US_TICK_SCALE 16
#define TICK_US_SCALE 10
#define US_TO_TICKS(us) (((uint32_t)(us)*ctx->ticksPerUs) >> US_TICK_SCALE)
#define TICKS_TO_US(t) (((uint32_t)(t)*ctx->usPerTick) >> TICK_US_SCALE)

STATIC_ASSERT(10000 <= RTC_SECOND_IN_US);
STATIC_ASSERT(RTC_SECOND_IN_US <= 200000);
STATIC_ASSERT(((1ULL * RTC_SECOND_IN_US) << US_TICK_SCALE) < 0xff000000);

typedef struct rtc_state {
    uint64_t microsecondBase;
    uint32_t ticksPerUs, usPerTick;
    uint16_t presc;
    cb_t cb;
    uint8_t lastSecond, needsSync;
} ctx_t;

static ctx_t ctx_;

#if 0
#define PIN_PWR_STATE PIN_P0
#define PIN_PWR_LOG PIN_P1
#else
#define PIN_PWR_STATE -1
#define PIN_PWR_LOG -1
#endif

#ifdef STM32G0
#define RTC_IRQn RTC_TAMP_IRQn
#define RTC_IRQHandler RTC_TAMP_IRQHandler
#define EXTI_LINE LL_EXTI_LINE_19
#elif defined(STM32WL)
#define RTC_IRQn RTC_Alarm_IRQn
#define RTC_IRQHandler RTC_Alarm_IRQHandler
#define EXTI_LINE LL_EXTI_LINE_17
#define LL_EXTI_ClearRisingFlag_0_31 LL_EXTI_ClearFlag_0_31
#define LL_PWR_ClearFlag_SB LL_PWR_ClearFlag_C1STOP_C1STB
#define LL_PWR_IsActiveFlag_SB LL_PWR_IsActiveFlag_C1SB
#else
#define EXTI_LINE LL_EXTI_LINE_17
#define LL_EXTI_ClearRisingFlag_0_31 LL_EXTI_ClearFlag_0_31
#endif

#define BCD(t, h) (((t)&0xf) + 10 * (((t) >> 4) & ((1 << h) - 1)))
uint32_t rtc_get_seconds(void) {
    uint32_t t = RTC->TR;
    uint32_t d = RTC->DR;
    DMESG("rtc %x %x", (unsigned)d, (unsigned)t);
    uint32_t d0 = (BCD(d >> 0, 2) - 1) + 30 * (BCD(d >> 8, 1) - 1) + 365 * BCD(d >> 16, 4);
    uint32_t r = BCD(t >> 0, 3) + 60 * (BCD(t >> 8, 3) + 60 * (BCD(t >> 16, 2) + 24 * d0));
    return r;
}

// ~20us at 8MHz
void rtc_sync_time() {
    ctx_t *ctx = &ctx_;

    if (!ctx->needsSync)
        return;

    target_disable_irq();
    uint16_t subsecond;
    uint8_t newSecond;
    for (;;) {
        subsecond = LL_RTC_TIME_GetSubSecond(RTC); // this locks TR/DR
        newSecond = RTC->TR & 0x7f;
        if (subsecond == LL_RTC_TIME_GetSubSecond(RTC) && (RTC->TR & 0x7f) == newSecond) {
            (void)RTC->DR; // unlock DR/TR
            break;
        }
    }
    subsecond = ctx->presc - subsecond;
    if (newSecond != ctx->lastSecond) {
        // this branch adds ~4us
        int diff = 60 + BCD(newSecond, 3) - BCD(ctx->lastSecond, 3);
        if (diff >= 60)
            diff -= 60;
        ctx->lastSecond = newSecond;
        ctx->microsecondBase += diff * RTC_SECOND_IN_US;
    }
    tim_set_micros(ctx->microsecondBase + TICKS_TO_US(subsecond));
    ctx->needsSync = false;
    target_enable_irq();
}

static void schedule_sync(ctx_t *ctx) {
    ctx->needsSync = true;
    uint64_t now = tim_get_micros();
    uint64_t delta = now - ctx->microsecondBase;
    if (delta > RTC_SECOND_IN_US * 50) {
        // we've been running for too long on regular timer - we lost sync with RTC
        uint16_t subsecond;
        uint8_t newSecond;
        for (;;) {
            subsecond = LL_RTC_TIME_GetSubSecond(RTC); // this locks TR/DR
            newSecond = RTC->TR & 0x7f;
            if (subsecond == LL_RTC_TIME_GetSubSecond(RTC) && (RTC->TR & 0x7f) == newSecond) {
                (void)RTC->DR; // unlock DR/TR
                break;
            }
        }
        subsecond = ctx->presc - subsecond;
        ctx->microsecondBase = tim_get_micros() - TICKS_TO_US(subsecond);
        ctx->lastSecond = newSecond;
        DMESG("RTC sync %d", (int)(delta));
    }
}

static void rtc_set(ctx_t *ctx, uint32_t delta_us, cb_t f) {
    if (delta_us > RTC_SECOND_IN_US)
        JD_PANIC();
    if (delta_us < RTC_MIN_TIME_US)
        delta_us = RTC_MIN_TIME_US;
    uint32_t delta_tick = US_TO_TICKS(delta_us);

    ctx->cb = f;

    LL_RTC_ALMA_Disable(RTC);
#ifndef STM32WL
    while (!LL_RTC_IsActiveFlag_ALRAW(RTC))
        ;
#endif

    uint32_t v = LL_RTC_TIME_GetSubSecond(RTC);
    (void)RTC->DR;                             // unlock DR/TR
    uint32_t nv = v + ctx->presc - delta_tick; // the timer counts back; also don't underflow
    if (nv >= ctx->presc)
        nv -= ctx->presc; // make sure it's in range

    LL_RTC_ALMA_SetSubSecond(RTC, nv);
    LL_RTC_ClearFlag_ALRA(RTC);
    LL_EXTI_ClearRisingFlag_0_31(EXTI_LINE);
    NVIC_ClearPendingIRQ(RTC_IRQn);
    LL_RTC_ALMA_Enable(RTC);
    // pin_pulse(PIN_PWR_LOG, 1);
}

void rtc_cancel_cb() {
    ctx_.cb = NULL;
}

void RTC_IRQHandler(void) {
    rtc_sync_time();

    if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
        LL_RTC_ClearFlag_ALRA(RTC);

        target_disable_irq();
        cb_t f = ctx_.cb;
        ctx_.cb = NULL;
        target_enable_irq();

        if (f)
            f();
    }

    // Clear the EXTI's Flag for RTC Alarm
    LL_EXTI_ClearRisingFlag_0_31(EXTI_LINE);
}

static void rtc_config(uint8_t p0, uint16_t p1) {
#ifdef STM32G0
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR | LL_APB1_GRP1_PERIPH_RTC);
#elif defined(STM32WL)
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);
    // LL_C2_APB1_GRP1_EnableClock(LL_C2_APB1_GRP1_PERIPH_RTCAPB);
#else
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif

    LL_PWR_EnableBkUpAccess();

    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();

#ifdef JD_USE_LSE
    LL_RCC_LSE_Enable();
    while (!LL_RCC_LSE_IsReady())
        ;
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
#else
    LL_RCC_LSI_Enable();
    while (!LL_RCC_LSI_IsReady())
        ;
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
#endif

    LL_RCC_EnableRTC();

    LL_RTC_DisableWriteProtection(RTC);

    LL_RTC_EnableInitMode(RTC);
    while (LL_RTC_IsActiveFlag_INIT(RTC) != 1)
        ;

    LL_RTC_EnableShadowRegBypass(RTC);

    LL_RTC_SetAsynchPrescaler(RTC, p0 - 1);
    LL_RTC_SetSynchPrescaler(RTC, p1 - 1);

    RTC->TR = 0;
    RTC->DR = 0x2101; // BCD! and one-based!
    RTC->SSR = 0;

    // set alarm
    LL_RTC_ALMA_SetMask(RTC, LL_RTC_ALMA_MASK_ALL); // ignore all

    LL_RTC_ALMA_SetSubSecond(RTC, 0);
    LL_RTC_ALMA_SetSubSecondMask(RTC, 15); // compare entire sub-second reg

    LL_RTC_ALMA_Enable(RTC); // only enabled alarm when requested
    LL_RTC_ClearFlag_ALRA(RTC);
    LL_RTC_EnableIT_ALRA(RTC);

    LL_EXTI_EnableIT_0_31(EXTI_LINE);
    LL_EXTI_EnableRisingTrig_0_31(EXTI_LINE);

    NVIC_SetPriority(RTC_IRQn, IRQ_PRIORITY_TIM); // match tim.c
    NVIC_EnableIRQ(RTC_IRQn);

    LL_RTC_DisableInitMode(RTC);
}

// with more than 4000 it overflows!
#define CALIB_CYCLES 1024

void rtc_init() {
    ctx_t *ctx = &ctx_;

    pin_setup_output(PIN_PWR_STATE);
    pin_setup_output(PIN_PWR_LOG);

#ifdef JD_USE_LSE
    // assume 32.768kHz crystal
    uint32_t d = 1000000 * CALIB_CYCLES / 32768;
#else
    target_disable_irq();
    rtc_config(1, CALIB_CYCLES);
    uint64_t t0 = tim_get_micros();
    while (LL_RTC_IsActiveFlag_ALRA(RTC) == 0)
        ;
    uint32_t d = (tim_get_micros() - t0) + 20;
    target_enable_irq();
#endif

    ctx->ticksPerUs = (1 << US_TICK_SCALE) * CALIB_CYCLES / d;
    ctx->usPerTick = d * CALIB_CYCLES / (1 << TICK_US_SCALE);

    int tmp = US_TO_TICKS(RTC_SECOND_IN_US);
#ifdef JD_USE_LSE
    // we get 1023 due to rounding...
    tmp++;
#endif
    uint32_t h_ms = US_TO_TICKS(1000000);
    DMESG("rtc: 1s=%d ticks; presc=%d", (unsigned)h_ms, tmp);
    // We're expecting around 40000, but there's large drift possible.
    // Values of 32500 and 51500 observed on the *same device* between power cycles,
    // however the value is stable between resets and while running.
    if (!(30000 <= h_ms && h_ms <= 55000))
        JD_PANIC();
    if (tmp > 0x7f00)
        JD_PANIC();
    ctx->presc = tmp;
    rtc_config(1, ctx->presc);
    LL_RTC_ALMA_Disable(RTC);
    ctx->needsSync = true;
    rtc_sync_time();
}

// standby stuff not currently used
void rtc_set_to_seconds_and_standby() {
    ctx_t *ctx = &ctx_;

    if (ctx->ticksPerUs) {
        int pr = US_TO_TICKS(1000000);
        int async_pr = 3;
        DMESG("pr=%d * %d", pr >> async_pr, 1 << async_pr);
        rtc_config(1 << async_pr, pr >> async_pr);
        LL_RTC_ALMA_Disable(RTC);
        LL_RTC_EnableWriteProtection(RTC);
    }

    //    pin_setup_input(PA_0, -1);
#if defined(STM32G0) || defined(STM32WL)
    LL_PWR_EnableGPIOPullDown(LL_PWR_GPIO_A, LL_PWR_GPIO_BIT_0);
    LL_PWR_EnablePUPDCfg();
#endif
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
    LL_PWR_ClearFlag_SB();
    LL_PWR_ClearFlag_WU();
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
    LL_LPM_EnableDeepSleep();
    __enable_irq(); // just in case, otherwise WFI will do nothing
    __WFI();
}

bool rtc_check_standby(void) {
#if defined(STM32G0) || defined(STM32WL)
    LL_PWR_DisablePUPDCfg();
#endif

    if (LL_PWR_IsActiveFlag_SB()) {
        LL_PWR_ClearFlag_SB();
        LL_PWR_ClearFlag_WU();
        return true;
    }
    return false;
}

void rtc_sleep(bool forceShallow) {
    if (forceShallow) {
        pin_pulse(PIN_PWR_LOG, 1);
        LL_RTC_ALMA_Disable(RTC);
        __WFI();
        return;
    }

    target_disable_irq();
    uint32_t usec;
    cb_t f = tim_steal_callback(&usec);
    if (!f) {
        target_enable_irq();
        pin_pulse(PIN_PWR_LOG, 2);
        LL_RTC_ALMA_Disable(RTC);
        __WFI();
        return;
    }

    rtc_set(&ctx_, usec, f);
#if defined(STM32G0) || defined(STM32WL)
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
#else
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP_LPREGU);
#endif

// enable to test power in stand by; should be around 3.2uA
#if 0
    LL_RTC_ALMA_Disable(RTC);
    pin_pulse(PIN_P1, 5);
    LL_PWR_ClearFlag_SB();
    LL_PWR_ClearFlag_WU();
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
#endif

    rtc_deepsleep();
}

void rtc_deepsleep(void) {
#ifdef STM32WL
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
#endif
    LL_LPM_EnableDeepSleep();
    pin_set(PIN_PWR_STATE, 0);
    schedule_sync(&ctx_);
    __WFI();
    target_enable_irq();
    rtc_sync_time();      // likely already happened in ISR, but doesn't hurt to check again
    LL_LPM_EnableSleep(); // i.e., no deep
}
