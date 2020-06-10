#include "jdprofile.h"

DEVICE_CLASS(0x32bffe5b, "JM D-Pad v1.0");

void app_init_services() {
    gamepad_init();
}
