#include "jdstm.h"

void pin_set(int pin, int v) {
    if ((uint8_t)pin == NO_PIN)
        return;
    if (v)
        LL_GPIO_SetOutputPin(PIN_PORT(pin), PIN_MASK(pin));
    else
        LL_GPIO_ResetOutputPin(PIN_PORT(pin), PIN_MASK(pin));
}

void pin_setup_output(int pin) {
    if ((uint8_t)pin == NO_PIN)
        return;

    GPIO_TypeDef *GPIOx = PIN_PORT(pin);
    uint32_t currentpin = PIN_MASK(pin);

    // Do it in the same order as LL_GPIO_Init() just in case
    LL_GPIO_SetPinSpeed(GPIOx, currentpin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(GPIOx, currentpin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOx, currentpin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOx, currentpin, LL_GPIO_MODE_OUTPUT);
}

void pin_set_opendrain(int pin) {
    if ((uint8_t)pin == NO_PIN)
        return;
    LL_GPIO_SetPinOutputType(PIN_PORT(pin), PIN_MASK(pin), LL_GPIO_OUTPUT_OPENDRAIN);
}

void pin_set_pull(int pin, int pull) {
    if ((uint8_t)pin == NO_PIN)
        return;
    LL_GPIO_SetPinPull(PIN_PORT(pin), PIN_MASK(pin),
                       pull == PIN_PULL_DOWN   ? LL_GPIO_PULL_DOWN
                       : pull == PIN_PULL_UP   ? LL_GPIO_PULL_UP
                       : pull == PIN_PULL_NONE ? LL_GPIO_PULL_NO
                                               : (hw_panic(), 0));
}

void pin_setup_input(int pin, int pull) {
    if ((uint8_t)pin == NO_PIN)
        return;
    GPIO_TypeDef *GPIOx = PIN_PORT(pin);
    uint32_t currentpin = PIN_MASK(pin);
    LL_GPIO_SetPinPull(GPIOx, currentpin,
                       pull == -1  ? LL_GPIO_PULL_DOWN
                       : pull == 1 ? LL_GPIO_PULL_UP
                       : pull == 0 ? LL_GPIO_PULL_NO
                                   : (hw_panic(), 0));
    LL_GPIO_SetPinMode(GPIOx, currentpin, LL_GPIO_MODE_INPUT);
}

void pin_setup_analog_input(int pin) {
    if ((uint8_t)pin == NO_PIN)
        return;

    GPIO_TypeDef *GPIOx = PIN_PORT(pin);
    uint32_t currentpin = PIN_MASK(pin);
    LL_GPIO_SetPinPull(GPIOx, currentpin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOx, currentpin, LL_GPIO_MODE_ANALOG);
}

// TODO use this everywhere
void pin_setup_output_af(int pin, int af) {
    if ((uint8_t)pin == NO_PIN)
        return;

    GPIO_TypeDef *GPIOx = PIN_PORT(pin);
    uint32_t currentpin = PIN_MASK(pin);

    LL_GPIO_SetPinSpeed(GPIOx, currentpin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(GPIOx, currentpin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOx, currentpin, LL_GPIO_PULL_DOWN);

    if (currentpin < LL_GPIO_PIN_8)
        LL_GPIO_SetAFPin_0_7(GPIOx, currentpin, af);
    else
        LL_GPIO_SetAFPin_8_15(GPIOx, currentpin, af);

    LL_GPIO_SetPinMode(GPIOx, currentpin, LL_GPIO_MODE_ALTERNATE);
}

void pin_pulse(int pin, int times) {
    if ((uint8_t)pin == NO_PIN)
        return;
    LL_GPIO_SetPinMode(PIN_PORT(pin), PIN_MASK(pin), LL_GPIO_MODE_OUTPUT);
    while (times--) {
        LL_GPIO_SetOutputPin(PIN_PORT(pin), PIN_MASK(pin));
        LL_GPIO_ResetOutputPin(PIN_PORT(pin), PIN_MASK(pin));
    }
}

void pin_toggle(int pin) {
    LL_GPIO_TogglePin(PIN_PORT(pin), PIN_MASK(pin));
}

int pin_get(int pin) {
    if ((uint8_t)pin == NO_PIN)
        return -1;
    return LL_GPIO_IsInputPinSet(PIN_PORT(pin), PIN_MASK(pin));
}
