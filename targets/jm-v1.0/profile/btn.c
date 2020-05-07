#include "jdprofile.h"

DEVICE_CLASS(0x325325f2, "JM BTN1 v1.0");

void init_services() {
    btn_init(PA_2);
}
