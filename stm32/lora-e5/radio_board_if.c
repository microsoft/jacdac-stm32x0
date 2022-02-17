#include "jdstm.h"
#include "radio_board_if.h"

#define PIN_LORA_CTRL1 PA_4
#define PIN_LORA_CTRL2 PA_5
#define PIN_LORA_CTRL3 NO_PIN

int32_t RBI_Init(void) {
    pin_setup_output(PIN_LORA_CTRL1);
    pin_setup_output(PIN_LORA_CTRL2);
    pin_setup_output(PIN_LORA_CTRL3);
    return 0;
}

int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef cfg) {
    switch (cfg) {
    case RBI_SWITCH_OFF:
        pin_set(PIN_LORA_CTRL3, 0);
        pin_set(PIN_LORA_CTRL1, 0);
        pin_set(PIN_LORA_CTRL2, 0);
        break;

    case RBI_SWITCH_RX:
        pin_set(PIN_LORA_CTRL3, 1);
        pin_set(PIN_LORA_CTRL1, 1);
        pin_set(PIN_LORA_CTRL2, 0);
        break;

    case RBI_SWITCH_RFO_LP: // Low-power TX
        pin_set(PIN_LORA_CTRL3, 1);
        pin_set(PIN_LORA_CTRL1, 1);
        pin_set(PIN_LORA_CTRL2, 1);
        break;

    case RBI_SWITCH_RFO_HP: // high-power TX
        pin_set(PIN_LORA_CTRL3, 1);
        pin_set(PIN_LORA_CTRL1, 0);
        pin_set(PIN_LORA_CTRL2, 1);
        break;

    default:
        jd_panic();
        break;
    }

    return 0;
}

int32_t RBI_GetTxConfig(void) {
    return RBI_CONF_RFO_HP; // _LP not supported
}

int32_t RBI_IsTCXO(void) {
    return 1;
}

int32_t RBI_IsDCDC(void) {
    return 1;
}
