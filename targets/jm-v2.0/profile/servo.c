#include "jdprofile.h"

DEVICE_CLASS(0x30b0c24e, "JM PWM (Servo) v2.0");

void app_init_services() {
    servo_init(PA_7);
}
