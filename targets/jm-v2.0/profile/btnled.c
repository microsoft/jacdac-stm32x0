#include "jdprofile.h"

DEVICE_CLASS(0x34dd5196, "JM ArcadeBtn v2.0");

void init_services() {
    btn_init(PA_10, PA_4);
}
