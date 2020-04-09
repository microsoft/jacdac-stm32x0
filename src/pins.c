#include "jdsimple.h"

#define PORT(pin) ((GPIO_TypeDef *)(GPIOA_BASE + (0x400 * (pin >> 4))))
#define PIN(pin) (1 << ((pin)&0xf))

void pin_setup_output(int pin) {
    if ((uint8_t)pin == 0xff)
        return;
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin = PIN(pin);
    LL_GPIO_Init(PORT(pin), &GPIO_InitStruct);
}

void pin_setup_input(int pin, int pull) {
    if ((uint8_t)pin == 0xff)
        return;
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    if (pull == -1)
        GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    else if (pull == 1)
        GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    else if (pull == 0)
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    else
        jd_panic();
    GPIO_InitStruct.Pin = PIN(pin);
    LL_GPIO_Init(PORT(pin), &GPIO_InitStruct);
}

void pin_setup_analog_input(int pin) {
    if ((uint8_t)pin == 0xff)
        return;
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin = PIN(pin);
    LL_GPIO_Init(PORT(pin), &GPIO_InitStruct);
}

// TODO use this everywhere
void pin_setup_output_af(int pin, int af) {
    if ((uint8_t)pin == 0xff)
        return;
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = PIN(pin);
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Alternate = af;
    LL_GPIO_Init(PORT(pin), &GPIO_InitStruct);
}

void pin_set(int pin, int v) {
    if ((uint8_t)pin == 0xff)
        return;
    if (v)
        LL_GPIO_SetOutputPin(PORT(pin), PIN(pin));
    else
        LL_GPIO_ResetOutputPin(PORT(pin), PIN(pin));
}

void pin_toggle(int pin) {
    LL_GPIO_TogglePin(PORT(pin), PIN(pin));
}

int pin_get(int pin) {
    if ((uint8_t)pin == 0xff)
        return -1;
    return LL_GPIO_IsInputPinSet(PORT(pin), PIN(pin));
}
