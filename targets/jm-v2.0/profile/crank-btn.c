#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x30838b8e, "JM Crank+Btn v2.0");

void app_init_services() {
    rotary_init(PA_6, PA_5, 24);
    btn_init(PA_4, 0, -1);
}
