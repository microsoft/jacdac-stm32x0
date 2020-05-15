#include "jdprofile.h"

DEVICE_CLASS(0x30a16d3c, "JM Power v1.0");

void init_services() {
    power_init();
}
