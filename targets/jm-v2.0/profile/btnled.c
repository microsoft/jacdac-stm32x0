#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x34dd5196, "JM ArcadeBtn v2.0");

void app_init_services() {
    btn_init(PA_10, 1, PA_4);
}
