#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x30a16d3c, "JM Power v1.0");

void app_init_services() {
    power_init(PA_4, PA_5, PA_6, PA_3);
}
