#include "jdprofile.h"

DEVICE_CLASS(0x33991b74, "JM PWM (npx) v1.0");

void init_services() {
    light_init();
}
