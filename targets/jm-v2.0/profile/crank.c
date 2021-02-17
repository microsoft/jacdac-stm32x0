#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x33a8780b, "JM Crank v2.0");

void app_init_services() {
    rotary_init(PA_6, PA_5, 24);
}
