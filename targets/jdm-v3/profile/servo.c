#include "jdsimple.h"

#define PIN_SERVO PA_6

SET_HW_TYPE(0x35bdb27f);

void init_services() {
    servo_init(PIN_SERVO);
}
