#include "blhw.h"
#include "jd_protocol.h"
#include "tinyhw.h"
#include "pinnames.h"

uint64_t hw_device_id(void) {
#ifdef STM32WL
    static uint64_t b;
    if (b)
        return b;
    uint64_t a;
    // 37.1.4 IEEE 64-bit unique device ID register (UID64)
    a = *(uint64_t *)0x1fff7580;
    uint8_t *src = (uint8_t *)&a;
    uint8_t *dst = (uint8_t *)&b;
    for (int i = 0; i < 8; ++i) {
        dst[i] = src[7 - i];
    }
    return b;
#else
    return APP_DEVICE_ID;
#endif
}

uint32_t app_get_device_class(void) {
    return app_dev_info.device_class;
}

void power_pin_enable(int en) {
    if (en) {
        pin_setup_output(PIN_PWR);
        pin_set(PIN_PWR, 0);
    } else {
        pin_setup_input(PIN_PWR, PIN_PULL_NONE);
    }
}
