#include "jdprofile.h"

DEVICE_CLASS(0x3d216fd4, "JDF030 btn v0");

void init_services() {
    btn_init(PA_4);
}
