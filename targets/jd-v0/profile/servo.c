#include "jdprofile.h"

#define PIN_SERVO PA_6

DEVICE_CLASS(0x3beb4448, "JDF030 servo v0");

void init_services() {
    servo_init(PIN_SERVO);
}
