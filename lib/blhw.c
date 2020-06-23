#include "blhw.h"
#include "jd_protocol.h"
#include "tinyhw.h"
#include "pinnames.h"
#include "board.h"

uint64_t hw_device_id(void) {
    return app_dev_info.device_id;
}

uint32_t app_get_device_class(void) {
    return app_dev_info.device_class;
}

void power_pin_enable(int en) {
#ifdef PWR_PIN_PULLUP
    if (en) {
        pin_setup_output(PIN_PWR);
        pin_set(PIN_PWR, 0);
    } else {
        pin_setup_input(PIN_PWR, 0);
    }
#else
    pin_set(PIN_PWR, !en);
#endif
}