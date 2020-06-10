#include "jdprofile.h"

DEVICE_CLASS(0x35643e91, "JM PWM (npx) v2.0");

void app_init_services() {
    light_init();
}
