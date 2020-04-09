#include "jdsimple.h"

#ifdef LOW_POWER
static volatile uint64_t last_alrm;
static volatile uint8_t is_led_off_alrm, was_led_off_alrm;
static cb_t cb;
static uint16_t presc, led_val;

void rtc_set_cb(cb_t f) {
    cb = f;
}

static void program_alrm(uint16_t val) {
    if (val >= presc - 1)
        return;

    // LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ALMA_Disable(RTC);
    while (!LL_RTC_IsActiveFlag_ALRAW(RTC))
        ;
    LL_RTC_ALMA_SetSubSecond(RTC, val);
    LL_RTC_ALMA_Enable(RTC);
    // LL_RTC_EnableWriteProtection(RTC);
}

void rtc_set_led_duty(int val) {
    int tmp = val * presc >> 10;
    if (tmp < 5)
        led_val = 0;
    else
        led_val = presc - tmp;
}

void RTC_IRQHandler(void) {
    pin_set(PIN_P1, 1);

    if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
        LL_RTC_ClearFlag_ALRA(RTC);

        if (is_led_off_alrm) {
            program_alrm(0);
            pin_set(PIN_P0, 0);
            led_set(0);
            is_led_off_alrm = 0;
            was_led_off_alrm = 1;
            // around 20us if pin access optimized
        } else {
            if (led_val) {
                is_led_off_alrm = 1;
                program_alrm(led_val);
                pin_set(PIN_P0, 1);
                led_set(1);
            }

            uint64_t now = tim_get_micros();
            if (last_alrm) {
                uint32_t d = now - last_alrm;
                // if it appears the timer was off while we were sleeping
                if (d < RTC_ALRM_US * 8 / 10) {
                    // move it forward
                    tim_forward(last_alrm + RTC_ALRM_US - now);
                    now = last_alrm + RTC_ALRM_US;
                }
            }
            last_alrm = now;

            target_disable_irq();
            cb_t f = cb;
            cb = NULL;
            target_enable_irq();

            was_led_off_alrm = 0;
            if (f)
                f();
        }
    }

    // Clear the EXTI's Flag for RTC Alarm
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
}

static void rtc_config(uint8_t p0, uint16_t p1) {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_PWR_EnableBkUpAccess();

    LL_RCC_LSI_Enable();

    while (LL_RCC_LSI_IsReady() != 1)
        ;

    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

    LL_RCC_EnableRTC();

    LL_RTC_DisableWriteProtection(RTC);

    LL_RTC_EnableInitMode(RTC);
    while (LL_RTC_IsActiveFlag_INIT(RTC) != 1)
        ;

    LL_RTC_SetAsynchPrescaler(RTC, p0 - 1);
    LL_RTC_SetSynchPrescaler(RTC, p1 - 1);

    // set alarm
    LL_RTC_ALMA_SetMask(RTC, LL_RTC_ALMA_MASK_ALL); // ignore all

    LL_RTC_ALMA_SetSubSecond(RTC, 0);
    LL_RTC_ALMA_SetSubSecondMask(RTC, 15); // compare entire sub-second reg

    LL_RTC_ALMA_Enable(RTC);
    LL_RTC_ClearFlag_ALRA(RTC);
    LL_RTC_EnableIT_ALRA(RTC);

    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);

    NVIC_SetPriority(RTC_IRQn, 2); // match tim.c
    NVIC_EnableIRQ(RTC_IRQn);

    LL_RTC_DisableInitMode(RTC);

    LL_RTC_ClearFlag_RS(RTC);
    while (LL_RTC_IsActiveFlag_RS(RTC) != 1)
        ;

    // LL_RTC_EnableWriteProtection(RTC);
}

#define CALIB_CYCLES 1024

void rtc_init() {
    rtc_config(1, CALIB_CYCLES);
    uint64_t t0 = tim_get_micros();
    target_disable_irq();
    while (LL_RTC_IsActiveFlag_ALRA(RTC) == 0)
        ;
    uint32_t d = (tim_get_micros() - t0) + 20;
    target_enable_irq();

    presc = CALIB_CYCLES * RTC_ALRM_US / d;
    DMESG("rtc: c=%d p=%d", d, presc);

    rtc_config(1, presc);
}

void setup_clock(void);

void rtc_sleep() {
    if (!presc) {
        __WFI();
        return;
    }

    // only go to sleep when JD asked us to run the tick()
    if (cb == NULL || jd_is_busy())
        return;

#if 0
    // 6uA
    rtc_config(100, 10000);
    LL_PWR_ClearFlag_SB();
    LL_PWR_ClearFlag_WU();
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
#else
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP_LPREGU);
    // LL_PWR_SetPowerMode(LL_PWR_MODE_STOP_MAINREGU);
#endif
    LL_LPM_EnableDeepSleep();

    for (;;) {
        pin_set(PIN_P1, 0);
        __WFI();
        if (!was_led_off_alrm)
            break;
    }
}
#endif