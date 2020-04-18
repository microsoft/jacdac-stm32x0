#include "jdprofile.h"

#define PIN_SERVO PA_6

DEVICE_CLASS(0x35bdb27f);

void init_services() {
    servo_init(PIN_SERVO);
}
