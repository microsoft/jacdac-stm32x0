#include "jdprofile.h"

DEVICE_CLASS(0x33498f7e, "JDM3 mono");

void app_init_services() {
    pwm_light_init(PIN_GLO1);
}
