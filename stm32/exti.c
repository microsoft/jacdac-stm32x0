#include "jdstm.h"

static cb_t callbacks[16];

static void _check_line(int ln) {
#if defined(STM32F0) || defined(STM32WL)
    LL_EXTI_ClearFlag_0_31(1 << ln);
#else
    LL_EXTI_ClearRisingFlag_0_31(1 << ln);
    LL_EXTI_ClearFallingFlag_0_31(1 << ln);
#endif
    callbacks[ln]();
}

#if defined(STM32G0)
#define EXTI_LINES() (EXTI->FPR1 | EXTI->RPR1)
#elif defined(STM32WL)
#define EXTI_LINES() (EXTI->PR1)
#else
#define EXTI_LINES() (EXTI->PR)
#endif

#define check_line(ln)                                                                             \
    if (lines & (1 << (ln)))                                                                       \
    _check_line(ln)

#ifdef STM32WL
#define SINGLE(name, ln)                                                                           \
    void name(void) {                                                                              \
        rtc_sync_time();                                                                           \
        uint32_t lines = EXTI_LINES();                                                             \
        check_line(ln);                                                                            \
    }
SINGLE(EXTI0_IRQHandler, 0)
SINGLE(EXTI1_IRQHandler, 1)
SINGLE(EXTI2_IRQHandler, 2)
SINGLE(EXTI3_IRQHandler, 3)
SINGLE(EXTI4_IRQHandler, 4)

void EXTI9_5_IRQHandler(void) {
    rtc_sync_time();
    uint32_t lines = EXTI_LINES();
    for (int i = 5; i <= 9; ++i)
        check_line(i);
}

void EXTI15_10_IRQHandler(void) {
    rtc_sync_time();
    uint32_t lines = EXTI_LINES();
    for (int i = 10; i <= 15; ++i)
        check_line(i);
}
#else
void EXTI0_1_IRQHandler(void) {
    rtc_sync_time();
    uint32_t lines = EXTI_LINES();
    check_line(0);
    check_line(1);
}

void EXTI2_3_IRQHandler(void) {
    rtc_sync_time();
    uint32_t lines = EXTI_LINES();
    check_line(2);
    check_line(3);
}

void EXTI4_15_IRQHandler(void) {
    rtc_sync_time();

    uint32_t lines = EXTI_LINES();

#ifdef UART_PIN
    // first check UART line, to speed up handling
    if (lines & (1 << (UART_PIN & 0xf))) {
        _check_line(UART_PIN & 0xf);
        lines = EXTI_LINES();
    }
#endif

    // check the rest of the lines (this includes UART line, but it's fine)
    for (int i = 4; i <= 15; ++i) {
        check_line(i);
    }
}
#endif

#ifdef STM32G0
static const uint32_t cfgs[] = {LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_PORTC};
#endif

void exti_set_callback(uint8_t pin, cb_t callback, uint32_t flags) {
    uint32_t extiport = 0;

    if (pin >> 4 > 2)
        JD_PANIC();
#if defined(STM32F0) || defined(STM32WL)
    extiport = pin >> 4;
#elif defined(STM32G0)
    extiport = cfgs[pin >> 4];
#else
#error "exti?"
#endif

    int pos = pin & 0xf;

#ifdef STM32F0
    uint32_t line = (pos >> 2) | ((pos & 3) << 18);
    LL_SYSCFG_SetEXTISource(extiport, line);
#elif defined(STM32WL)
    uint32_t line = (pos >> 2) | (0xF << (16 + (4 * (pos & 3))));
    LL_SYSCFG_SetEXTISource(extiport, line);
#else
    uint32_t line = (pos >> 2) | ((pos & 3) << 19);
    LL_EXTI_SetEXTISource(extiport, line);
#endif
    if (flags & EXTI_FALLING)
        LL_EXTI_EnableFallingTrig_0_31(1 << pos);
    if (flags & EXTI_RISING)
        LL_EXTI_EnableRisingTrig_0_31(1 << pos);
    LL_EXTI_EnableIT_0_31(1 << pos);

    callbacks[pos] = callback;

#define SETUP(irq)                                                                                 \
    NVIC_SetPriority(irq, IRQ_PRIORITY_EXTI);                                                      \
    NVIC_EnableIRQ(irq)

#ifdef STM32WL
    if (!NVIC_GetEnableIRQ(EXTI0_IRQn)) {
        SETUP(EXTI0_IRQn);
        SETUP(EXTI1_IRQn);
        SETUP(EXTI2_IRQn);
        SETUP(EXTI3_IRQn);
        SETUP(EXTI4_IRQn);
        SETUP(EXTI9_5_IRQn);
        SETUP(EXTI15_10_IRQn);
    }
#else
    if (!NVIC_GetEnableIRQ(EXTI0_1_IRQn)) {
        SETUP(EXTI0_1_IRQn);
        SETUP(EXTI2_3_IRQn);
        SETUP(EXTI4_15_IRQn);
    }
#endif
}