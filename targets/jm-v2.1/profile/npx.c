#include "jdprofile.h"

DEVICE_CLASS(0x3c6bebe4, "JM PWM+PWR (npx) v2.1");

void app_init_services() {
    pin_setup_output(PF_1);
    pin_set(PF_1, 0); // enable JD power
    // The sense lines are not connected on this hw revision, so we just skip the actual power service.
    light_init();
}
