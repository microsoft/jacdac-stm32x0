#include "jdprofile.h"

DEVICE_CLASS(0x3df48ce9, "JM Servo v2.0");

void init_services() {
    servo_init(PA_7);
}
