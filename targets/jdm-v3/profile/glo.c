#include "jdprofile.h"

DEVICE_CLASS(0x3fb97809);

void init_services() {
    pwm_light_init(PIN_GLO1);
}
