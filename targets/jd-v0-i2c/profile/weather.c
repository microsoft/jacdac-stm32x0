#include "jdprofile.h"

DEVICE_CLASS(0x379c2450, "JDF030 weather v0");

void init_services() {
    temp_init();
    humidity_init();
}
