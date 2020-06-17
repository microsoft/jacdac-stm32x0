#include "jdstm.h"

struct TimDesc {
    TIM_TypeDef *tim;
    uint8_t apb;
    uint32_t clkmask;
};

static const struct TimDesc tims[] = {
    {TIM1, 2, RCC_APB2ENR_TIM1EN}, //
#ifdef TIM2
    {TIM2, 1, RCC_APB1ENR_TIM2EN},
#endif
    {TIM3, 1, RCC_APB1ENR_TIM3EN},   //
    {TIM14, 1, RCC_APB1ENR_TIM14EN}, //
    {TIM16, 2, RCC_APB2ENR_TIM16EN}, //
    // {TIM17, 2, RCC_APB2ENR_TIM17EN}, // used in tim.c, skip here
};

struct PinPWM {
    uint8_t pin;
    uint8_t ch;
    uint8_t af;
    TIM_TypeDef *tim;
};

static const struct PinPWM pins[] = {
#ifdef TIM2
    {PA_1, 2, LL_GPIO_AF_2, TIM2},  // LED on jdm-v2
    {PA_3, 4, LL_GPIO_AF_2, TIM2},  // POWER on jdm-v2
    {PA_15, 1, LL_GPIO_AF_2, TIM2}, // LED on jdm-v3 (TIM2_CH1_ETR?)
#endif
    //{PA_6, 1, LL_GPIO_AF_5, TIM16}, // SERVO on jdm-v2,3 - doesn't seem to work, TIM3 works
    {PA_6, 1, LL_GPIO_AF_1, TIM3},  // SERVO on jdm-v2,3
    {PA_11, 4, LL_GPIO_AF_2, TIM1}, // POWER on jdm-v3
    {PB_0, 3, LL_GPIO_AF_1, TIM3},  // GLO0 on jdm-v3, also TIM1:2N
    {PB_1, 1, LL_GPIO_AF_0, TIM14}, // GLO1 on jdm-v3; also TIM3:4, TIM1:3N
    {PA_10, 3, LL_GPIO_AF_2, TIM1}, // SND
    {PA_4, 1, LL_GPIO_AF_4, TIM14}, // SND
    {PA_7, 1, LL_GPIO_AF_4, TIM14}, // servo
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
    jd_panic();
    return NULL;
}

// prescaler=1 -> 8MHz; prescaler=8 -> 1MHz
uint8_t pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler) {
    const struct PinPWM *pwm = lookup_pwm(pin);
    if (!pwm)
        jd_panic();

    TIM_TypeDef *TIMx = pwm->tim;

    if (prescaler == 0) prescaler = 1;

    const struct TimDesc *td = lookup_tim(TIMx);
    if (td->apb == 1) {
        SET_BIT(RCC->APB1ENR, td->clkmask);
        (void)READ_BIT(RCC->APB1ENR, td->clkmask); // delay
    } else if (td->apb == 2) {
        SET_BIT(RCC->APB2ENR, td->clkmask);
        (void)READ_BIT(RCC->APB2ENR, td->clkmask); // delay
    } else {
        jd_panic();
    }

    LL_TIM_DisableCounter(TIMx);

    pin_setup_output_af(pin, pwm->af);

    LL_TIM_OC_InitTypeDef tim_oc_initstruct;

    TIMx->CR1 = LL_TIM_COUNTERMODE_UP | LL_TIM_CLOCKDIVISION_DIV1; // default anyways

    LL_TIM_SetPrescaler(TIMx, prescaler - 1);
    LL_TIM_SetAutoReload(TIMx, period - 1);

    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_EnableARRPreload(TIMx);

    tim_oc_initstruct.OCMode = LL_TIM_OCMODE_PWM1;
    tim_oc_initstruct.OCState = LL_TIM_OCSTATE_DISABLE;
    tim_oc_initstruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    tim_oc_initstruct.CompareValue = duty;
    tim_oc_initstruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    tim_oc_initstruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    tim_oc_initstruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    tim_oc_initstruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;

    if (TIMx == TIM1)
        TIMx->BDTR |= TIM_BDTR_MOE;

    // do a switch, so the compiler can optimize bodies of these inline functions
    switch (pwm->ch) {
    case 1:
        LL_TIM_OC_Init(TIMx, LL_TIM_CHANNEL_CH1, &tim_oc_initstruct);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH1);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH1);
        break;
    case 2:
        LL_TIM_OC_Init(TIMx, LL_TIM_CHANNEL_CH2, &tim_oc_initstruct);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH2);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH2);
        break;
    case 3:
        LL_TIM_OC_Init(TIMx, LL_TIM_CHANNEL_CH3, &tim_oc_initstruct);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH3);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH3);
        break;
    case 4:
        LL_TIM_OC_Init(TIMx, LL_TIM_CHANNEL_CH4, &tim_oc_initstruct);
        LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH4);
        LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH4);
        break;
    }

    LL_TIM_EnableCounter(TIMx);
    LL_TIM_GenerateEvent_UPDATE(TIMx);

    return pwm - pins + 1;
}

void pwm_set_duty(uint8_t pwm, uint32_t duty) {
    pwm--;
    if (pwm >= sizeof(pins) / sizeof(pins[0]))
        jd_panic();
    WRITE_REG(*(&pins[pwm].tim->CCR1 + pins[pwm].ch - 1), duty);
}

void pwm_enable(uint8_t pwm, bool enabled) {
    pwm--;
    if (pwm >= sizeof(pins) / sizeof(pins[0]))
        jd_panic();
    if (enabled)
        pin_setup_output_af(pins[pwm].pin, pins[pwm].af);
    else
        pin_setup_output(pins[pwm].pin);
}
