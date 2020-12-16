#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x30b0c24e, "JM PWM (Servo) v2.0");

void app_init_services() {
    servo_init(PA_7);
}
