#include "jdprofile.h"

DEVICE_CLASS(0x33a8780b, "JM Crank v2.0");

void init_services() {
    crank_init(PA_6, PA_5);
}
