#include "jdprofile.h"

DEVICE_CLASS(0x3ac74e57, "JM PWM+SERVO v2.1");

void app_init_services() {
#if 1
    pin_setup_output(PF_1);
    pin_set(PF_1, 0); // enable JD power
#endif
    
    servo_init(PA_7);
    
    // The sense lines are not connected on this hw revision, so we just skip the actual power service.
}
