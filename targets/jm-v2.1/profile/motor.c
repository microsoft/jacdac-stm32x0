#include "jdprofile.h"

DEVICE_CLASS(0x3041ea56, "JM Motor v2.1");

void init_services() {
    motor_init(PA_4, PA_6, PA_3);
}
