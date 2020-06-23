#include "jdprofile.h"

#define PIN_SERVO PA_6

DEVICE_CLASS(0x3a1a3a06, "JDM3 servo");

void app_init_services() {
    servo_init(PIN_SERVO);
}
