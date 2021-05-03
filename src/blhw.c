#include "blhw.h"
#include "jd_protocol.h"
#include "tinyhw.h"
#include "pinnames.h"

uint64_t hw_device_id(void) {
    return APP_DEVICE_ID;
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
