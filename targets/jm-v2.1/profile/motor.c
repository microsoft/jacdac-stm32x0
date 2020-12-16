#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x3041ea56, "JM Motor v2.1");

void app_init_services() {
    motor_init(PA_4, PA_6, PA_3);
}
