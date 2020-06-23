#include "jdprofile.h"

DEVICE_CLASS(0x3beb4448, "JDM3 crank");

void app_init_services() {
    crank_init(PIN_P0, PIN_P1);
}
