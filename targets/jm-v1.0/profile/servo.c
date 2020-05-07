#include "jdprofile.h"

DEVICE_CLASS(0x3142fd1f, "JM PWM (servo) v1.0");

void init_services() {
    servo_init(PIN_SERVO);
}
