#include "jdstm.h"
#include "utilities_def.h"
#include "stm32_timer.h"
#include "stm32_systime.h"

#define LOG JD_LOG
#define VLOG JD_NOLOG

/**
 * @brief Minimum timeout delay of Alarm in ticks
 */
#define MIN_ALARM_DELAY 2

static uint32_t RtcTimerContext = 0;
static uint32_t user_timeout;
#define TIMEOUT_UNINIT 0
#define TIMEOUT_DISABLED 1
#define TIMEOUT_ENABLED 2
static uint8_t timeout_status;

#define ENTER_CRITICAL_SECTION()                                                                   \
    uint32_t primask_bit = __get_PRIMASK();                                                        \
    __disable_irq()

#define EXIT_CRITICAL_SECTION() __set_PRIMASK(primask_bit)

static uint32_t ticks_off;
static uint16_t prev_cnt;

static uint32_t GetTimerTicks(void) {
    ENTER_CRITICAL_SECTION();
    uint16_t curr_cnt = LPTIM1->CNT;
    if (curr_cnt < prev_cnt)
        ticks_off += 2000;
    prev_cnt = curr_cnt;
    return ticks_off + ((curr_cnt * 1000) >> 15);
    EXIT_CRITICAL_SECTION();
}

UTIL_TIMER_Status_t TIMER_IF_StopTimer(void) {
    timeout_status = TIMEOUT_DISABLED;
    return UTIL_TIMER_OK;
}

uint32_t TIMER_IF_SetTimerContext(void) {
    RtcTimerContext = GetTimerTicks();
    return RtcTimerContext;
}

UTIL_TIMER_Status_t TIMER_IF_Init(void) {
    if (timeout_status != TIMEOUT_UNINIT)
        return UTIL_TIMER_OK;

    LL_RCC_LSE_Enable();
    while (!LL_RCC_LSE_IsReady())
        ;

    LL_RCC_LSE_EnablePropagation();
    while (!LL_RCC_LSE_IsPropagationReady())
        ;

    LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
    LL_APB1_GRP1_EnableClockSleep(LL_APB1_GRP1_PERIPH_LPTIM1);

    NVIC_SetPriority(LPTIM1_IRQn, IRQ_PRIORITY_LORA);
    NVIC_EnableIRQ(LPTIM1_IRQn);

    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29); // LPTIM1
    LL_LPTIM_EnableIT_CMPM(LPTIM1);

#if 0
    // these are defaults anyways
    LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
    LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV1);
    LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
    LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
    LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
    LL_LPTIM_TrigSw(LPTIM1);
#endif

    LL_LPTIM_Enable(LPTIM1);

    // auto-reload = 2s
    LL_LPTIM_ClearFlag_ARROK(LPTIM1);
    LL_LPTIM_SetAutoReload(LPTIM1, 0xffff);
    while (!LL_LPTIM_IsActiveFlag_ARROK(LPTIM1))
        ;

    LL_LPTIM_ClearFlag_CMPOK(LPTIM1);
    LL_LPTIM_SetCompare(LPTIM1, 0x8000);
    while (!LL_LPTIM_IsActiveFlag_CMPOK(LPTIM1))
        ;

    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

    TIMER_IF_SetTimerContext();
    TIMER_IF_StopTimer();

    return UTIL_TIMER_OK;
}

static void set_hw_alarm(void) {
    ENTER_CRITICAL_SECTION();
    uint32_t now = GetTimerTicks();
    uint32_t delta;
    if (timeout_status == TIMEOUT_DISABLED)
        delta = 0x8000;
    else if (is_before(user_timeout, now))
        delta = 5;
    else {
        delta = ((user_timeout - now) << 15) / 1000;
        if (delta > 0x8000)
            delta = 0x7000;
        else if (delta < 5)
            delta = 5;
    }
    LPTIM1->CMP = prev_cnt + delta;
    EXIT_CRITICAL_SECTION();
    VLOG("set hw %d + %d (for %d ms)", prev_cnt, delta,
         timeout_status == TIMEOUT_DISABLED ? 0 : user_timeout - now);
}

UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout) {
    timeout_status = TIMEOUT_ENABLED;
    user_timeout = timeout + RtcTimerContext;
    VLOG("set tm=%d now=%d", user_timeout, GetTimerTicks());
    set_hw_alarm();
    return UTIL_TIMER_OK;
}

uint32_t TIMER_IF_GetTimerContext(void) {
    return RtcTimerContext;
}

uint32_t TIMER_IF_GetTimerElapsedTime(void) {
    return ((uint32_t)(GetTimerTicks() - RtcTimerContext));
}

uint32_t TIMER_IF_GetTimerValue(void) {
    if (timeout_status)
        return GetTimerTicks();
    else
        return 0;
}

uint32_t TIMER_IF_GetMinimumTimeout(void) {
    return MIN_ALARM_DELAY;
}

uint32_t TIMER_IF_Convert_ms2Tick(uint32_t timeMilliSec) {
    return timeMilliSec;
}

uint32_t TIMER_IF_Convert_Tick2ms(uint32_t tick) {
    return tick;
}

bool jd_lora_in_timer(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) == LPTIM1_IRQn + 16;
}

void LPTIM1_IRQHandler(void) {
    if (LL_LPTIM_IsActiveFlag_CMPM(LPTIM1)) {
        LL_LPTIM_ClearFLAG_CMPM(LPTIM1);
        uint32_t now = GetTimerTicks();
        VLOG("cmp %d %d", user_timeout, now);
        if (timeout_status == TIMEOUT_ENABLED && is_before(user_timeout, now)) {
            timeout_status = TIMEOUT_DISABLED;
            uint32_t st = tim_get_micros();
            LOG("run at %d (%d ms late)", (int)now, (int)now - (int)user_timeout);
            UTIL_TIMER_IRQ_Handler();
            LOG("duration %d us", (int)((uint32_t)tim_get_micros() - st));
        }
        set_hw_alarm();
    }
}

const UTIL_TIMER_Driver_s UTIL_TimerDriver = {
    TIMER_IF_Init,
    NULL,

    TIMER_IF_StartTimer,
    TIMER_IF_StopTimer,

    TIMER_IF_SetTimerContext,
    TIMER_IF_GetTimerContext,

    TIMER_IF_GetTimerElapsedTime,
    TIMER_IF_GetTimerValue,
    TIMER_IF_GetMinimumTimeout,

    TIMER_IF_Convert_ms2Tick,
    TIMER_IF_Convert_Tick2ms,
};
