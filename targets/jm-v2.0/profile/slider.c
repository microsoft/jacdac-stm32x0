#include "jdprofile.h"

DEVICE_CLASS(0x3e344e1d, "JM Slider v2.0");

void init_services() {
    slider_init(PA_3, PA_5, PA_4);
}
