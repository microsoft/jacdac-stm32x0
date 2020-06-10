#include "jdprofile.h"

DEVICE_CLASS(0x30838b8e, "JM Crank+Btn v2.0");

void app_init_services() {
    crank_init(PA_6, PA_5);
    btn_init(PA_4, -1);
}
