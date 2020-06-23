#include "jdprofile.h"

#define PIN_SERVO PA_6

DEVICE_CLASS(0x3faf16db, "JDF030 servo v0");

void app_init_services() {
    servo_init(PIN_SERVO);
}
