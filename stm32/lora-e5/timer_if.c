#include "jdstm.h"
#include "utilities_def.h"
#include "stm32_timer.h"
#include "stm32_systime.h"

/**
 * @brief Minimum timeout delay of Alarm in ticks
 */
#define MIN_ALARM_DELAY 2

static uint32_t RtcTimerContext = 0;
static uint32_t user_timeout;
#define TIMEOUT_UNINIT 0
#define TIMEOUT_DISABLED 1
#define TIMEOUT_THIS 1
#define TIMEOUT_LATER 2
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

    LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);

    NVIC_SetPriority(LPTIM1_IRQn, 3);
    NVIC_EnableIRQ(LPTIM1_IRQn);

    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29); // LPTIM1

    LL_LPTIM_EnableIT_CMPM(LPTIM1);

    // LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
    LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV32);
    // LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
    // LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
    // LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
    // LL_LPTIM_TrigSw(LPTIM1);

    LL_LPTIM_Enable(LPTIM1);

    // auto-reload = 2s
    LL_LPTIM_SetAutoReload(LPTIM1, 0xffff);
    LL_LPTIM_SetCompare(LPTIM1, 0x8000);

    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

    TIMER_IF_SetTimerContext();
    TIMER_IF_StopTimer();

    return UTIL_TIMER_OK;
}

static void set_hw_alarm(void) {
    ENTER_CRITICAL_SECTION();
    uint32_t now = GetTimerTicks();
    uint32_t delta;
    if (is_before(user_timeout, now))
        delta = 10;
    else if (timeout_status == TIMEOUT_DISABLED)
        delta = 0xe000;
    else {
        delta = ((user_timeout - now) << 15) / 1000;
        if (delta > 0xf000) {
            delta = 0xe000;
            timeout_status = TIMEOUT_LATER;
        } else {
            timeout_status = TIMEOUT_THIS;
        }
    }
    LPTIM1->CMP = prev_cnt + delta;
    EXIT_CRITICAL_SECTION();
}

UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout) {
    user_timeout = timeout;
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

void LPTIM1_IRQHandler(void) {
    uint32_t now = GetTimerTicks();
    if (timeout_status == TIMEOUT_DISABLED)
        return;
    if (is_before(user_timeout, now) || timeout_status == TIMEOUT_THIS) {
        timeout_status = TIMEOUT_DISABLED;
        UTIL_TIMER_IRQ_Handler();
    } else {
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
