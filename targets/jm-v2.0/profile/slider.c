#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x3e344e1d, "JM Slider v2.0");

void app_init_services() {
    potentiometer_init(PA_3, PA_5, PA_4);
}
