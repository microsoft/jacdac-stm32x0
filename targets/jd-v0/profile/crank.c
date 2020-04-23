#include "jdprofile.h"

DEVICE_CLASS(0x3f7c8355, "JDF030 crank v0");

void init_services() {
    crank_init(PIN_P0, PIN_P1);
}
