#include "jdstm.h"

struct TimDesc {
    TIM_TypeDef *tim;
    uint8_t apb;
    uint32_t clkmask;
};

static const struct TimDesc tims[] = {
#ifdef STM32G0
#define APB1ENR APBENR1
#define APB2ENR APBENR2
    {TIM1, 2, RCC_APBENR2_TIM1EN},   //
    {TIM14, 2, RCC_APBENR2_TIM14EN}, //
    {TIM16, 2, RCC_APBENR2_TIM16EN}, //
    {TIM17, 2, RCC_APBENR2_TIM17EN}, //
    {TIM3, 1, RCC_APBENR1_TIM3EN},   //
#ifdef TIM2
    {TIM2, 1, RCC_APBENR1_TIM2EN},
#endif
#elif defined(STM32F0)
    {TIM1, 2, RCC_APB2ENR_TIM1EN}, //
#ifdef TIM2
    {TIM2, 1, RCC_APB1ENR_TIM2EN},
#endif
    {TIM3, 1, RCC_APB1ENR_TIM3EN},   //
    {TIM14, 1, RCC_APB1ENR_TIM14EN}, //
    {TIM16, 2, RCC_APB2ENR_TIM16EN}, //
#elif defined(STM32WL)
#define APB1ENR APB1ENR1
    // TODO LPTIM...
    {TIM1, 2, RCC_APB2ENR_TIM1EN},  //
    {TIM2, 1, RCC_APB1ENR1_TIM2EN}, //
#else
#error "missing"
#endif
    // TIM17 // used in tim.c, skip here
};

struct PinPWM {
    uint8_t pin;
    uint8_t ch;
    uint8_t af;
    TIM_TypeDef *tim;
};

static const struct PinPWM pins[] = {
#ifdef STM32G0
#ifdef TIM2
    {PA_0, 1, LL_GPIO_AF_2, TIM2}, // rgb led
    {PA_1, 2, LL_GPIO_AF_2, TIM2}, // rgb led
    {PA_2, 3, LL_GPIO_AF_2, TIM2}, // rgb led
    {PA_3, 4, LL_GPIO_AF_2, TIM2}, // rgb led
#endif
#ifndef SYSTIM_ON_TIM14
    {PA_4, 1, LL_GPIO_AF_4, TIM14}, // PWM mikrobus
#endif
    {PA_6, 1, LL_GPIO_AF_1, TIM3},  // rgb led
    {PA_7, 2, LL_GPIO_AF_1, TIM3},  // rgb led
    {PB_0, 3, LL_GPIO_AF_1, TIM3},  // rgb led
    {PB_1, 4, LL_GPIO_AF_1, TIM3},  // rgb led
#ifdef SYSTIM_ON_TIM14
    {PB_9, 1, LL_GPIO_AF_2, TIM17}, // rgb led
#endif
    {PB_8, 1, LL_GPIO_AF_2, TIM16}, // rgb led
    {PA_10, 3, LL_GPIO_AF_2, TIM1}, //
    {PA_8, 1, LL_GPIO_AF_2, TIM1},  // rotary
    {PA_9, 2, LL_GPIO_AF_2, TIM1},  // rotary
#elif defined(STM32F0)
#ifdef TIM2
    {PA_1, 2, LL_GPIO_AF_2, TIM2},   // LED on jdm-v2
    {PA_3, 4, LL_GPIO_AF_2, TIM2},   // POWER on jdm-v2
    {PA_15, 1, LL_GPIO_AF_2, TIM2},  // LED on jdm-v3 (TIM2_CH1_ETR?)
#endif
    //{PA_6, 1, LL_GPIO_AF_5, TIM16}, // SERVO on jdm-v2,3 - doesn't seem to work, TIM3 works
    {PA_6, 1, LL_GPIO_AF_1, TIM3},  // SERVO on jdm-v2,3
    {PA_9, 2, LL_GPIO_AF_2, TIM1},  // G of RGB LED on XAC module
    {PA_11, 4, LL_GPIO_AF_2, TIM1}, // POWER on jdm-v3
    {PB_0, 3, LL_GPIO_AF_1, TIM3},  // GLO0 on jdm-v3, also TIM1:2N
    {PB_1, 4, LL_GPIO_AF_1, TIM3},  // GLO1 on jdm-v3; also TIM3:4, TIM1:3N
    //{PB_1, 1, LL_GPIO_AF_0, TIM14}, // GLO1 on jdm-v3; also TIM3:4, TIM1:3N
    {PA_10, 3, LL_GPIO_AF_2, TIM1}, // SND
    {PA_4, 1, LL_GPIO_AF_4, TIM14}, // SND
    {PA_7, 1, LL_GPIO_AF_4, TIM14}, // servo
#elif defined(STM32WL)
    {PA_0, 1, LL_GPIO_AF_1, TIM2},
    {PA_3, 4, LL_GPIO_AF_1, TIM2},
    {PB_10, 3, LL_GPIO_AF_1, TIM2},
// ...
#else
#error "missing"
#endif
};

static const struct PinPWM *lookup_pwm(uint8_t pin) {
    for (unsigned i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i)
        if (pins[i].pin == pin)
            return &pins[i];
    return NULL;
}

static const struct TimDesc *lookup_tim(TIM_TypeDef *tim) {
    for (unsigned i = 0; i < sizeof(tims) / sizeof(tims[0]); ++i)
        if (tims[i].tim == tim)
            return &tims[i];
    JD_PANIC();
    return NULL;
}

// prescaler=1 -> 8MHz; prescaler=8 -> 1MHz
uint8_t jd_pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler) {
    const struct PinPWM *pwm = lookup_pwm(pin);
    if (!pwm)
        JD_PANIC();

    TIM_TypeDef *TIMx = pwm->tim;

    if (prescaler == 0)
        prescaler = 1;

    const struct TimDesc *td = lookup_tim(TIMx);
    if (td->apb == 1) {
        SET_BIT(RCC->APB1ENR, td->clkmask);
        (void)READ_BIT(RCC->APB1ENR, td->clkmask); // delay
    } else if (td->apb == 2) {
        SET_BIT(RCC->APB2ENR, td->clkmask);
        (void)READ_BIT(RCC->APB2ENR, td->clkmask); // delay
    } else {
        JD_PANIC();
    }

    LL_TIM_DisableCounter(TIMx);

    pin_setup_output_af(pin, pwm->af);

    TIMx->CR1 = LL_TIM_COUNTERMODE_UP | LL_TIM_CLOCKDIVISION_DIV1; // default anyways

    LL_TIM_SetPrescaler(TIMx, prescaler - 1);
    LL_TIM_SetAutoReload(TIMx, period - 1);

    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_EnableARRPreload(TIMx);

    if (TIMx == TIM1 || TIMx == TIM16 || TIMx == TIM17)
        TIMx->BDTR |= TIM_BDTR_MOE;

    uint32_t mode = LL_TIM_OCMODE_PWM1;

    // do a switch, so the compiler can optimize bodies of these inline functions
    switch (pwm->ch) {
    case 1:
        LL_TIM_OC_SetMode(TIMx, LL_TIM_CHANNEL_CH1, mode);
        LL_TIM_OC_SetCompareCH1(TIMx, duty);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH1);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH1);
        break;
    case 2:
        LL_TIM_OC_SetMode(TIMx, LL_TIM_CHANNEL_CH2, mode);
        LL_TIM_OC_SetCompareCH2(TIMx, duty);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH2);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH2);
        break;
    case 3:
        LL_TIM_OC_SetMode(TIMx, LL_TIM_CHANNEL_CH3, mode);
        LL_TIM_OC_SetCompareCH3(TIMx, duty);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH3);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH3);
        break;
    case 4:
        LL_TIM_OC_SetMode(TIMx, LL_TIM_CHANNEL_CH4, mode);
        LL_TIM_OC_SetCompareCH4(TIMx, duty);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH4);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH4);
        break;
    }

    LL_TIM_EnableCounter(TIMx);
    LL_TIM_GenerateEvent_UPDATE(TIMx);

    return pwm - pins + 1;
}

void jd_pwm_set_duty(uint8_t pwm, uint32_t duty) {
    pwm--;
    if (pwm >= sizeof(pins) / sizeof(pins[0]))
        JD_PANIC();
    WRITE_REG(*(&pins[pwm].tim->CCR1 + pins[pwm].ch - 1), duty);
}

void jd_pwm_enable(uint8_t pwm, bool enabled) {
    pwm--;
    if (pwm >= sizeof(pins) / sizeof(pins[0]))
        JD_PANIC();
    if (enabled)
        pin_setup_output_af(pins[pwm].pin, pins[pwm].af);
    else
        pin_setup_output(pins[pwm].pin);
}

#ifdef STM32G0
// This kind-of works, but it seems the TIM still only runs when we're not sleeping so we miss quick
// turns Thus we just stick to software for now.
uint8_t encoder_init(uint8_t pinA, uint8_t pinB) {
    uint8_t idxA = jd_pwm_init(pinA, 0x10000, 0, 1) - 1;
    uint8_t idxB = jd_pwm_init(pinB, 0x10000, 0, 1) - 1;
    if (pins[idxA].tim != pins[idxB].tim)
        JD_PANIC();
    if (pins[idxA].ch != 1 || pins[idxB].ch != 2)
        JD_PANIC();

    TIM_TypeDef *TIMx = pins[idxA].tim;

    // enable sleep-mode clock
    const struct TimDesc *td = lookup_tim(TIMx);
    if (td->apb == 1) {
        SET_BIT(RCC->APBSMENR1, td->clkmask);
        (void)READ_BIT(RCC->APBSMENR1, td->clkmask); // delay
    } else if (td->apb == 2) {
        SET_BIT(RCC->APBSMENR2, td->clkmask);
        (void)READ_BIT(RCC->APBSMENR2, td->clkmask); // delay
    } else {
        JD_PANIC();
    }

    LL_TIM_CC_DisableChannel(TIMx, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2);

    LL_TIM_DisableCounter(TIMx);

    LL_TIM_SetCounter(TIMx, 0x8000);

    // LL_TIM_DisableARRPreload(TIMx);

    LL_TIM_SetEncoderMode(TIMx, LL_TIM_ENCODERMODE_X4_TI12);

    uint32_t config = LL_TIM_ACTIVEINPUT_DIRECTTI | LL_TIM_ICPSC_DIV1 | LL_TIM_IC_FILTER_FDIV1 |
                      LL_TIM_IC_POLARITY_RISING;

    LL_TIM_IC_Config(TIMx, LL_TIM_CHANNEL_CH1, config);
    LL_TIM_IC_Config(TIMx, LL_TIM_CHANNEL_CH2, config);

    LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2);

    LL_TIM_EnableCounter(TIMx);
    LL_TIM_GenerateEvent_UPDATE(TIMx);

    return idxA + 1;
}

uint32_t encoder_get(uint8_t pwm) {
    pwm--;
    if (pwm >= sizeof(pins) / sizeof(pins[0]))
        JD_PANIC();
    return LL_TIM_GetCounter(pins[pwm].tim);
}

#endif