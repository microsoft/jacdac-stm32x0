#include "jdprofile.h"

DEVICE_CLASS(0x36ed0f96, "JM TOUCH v1.0");

void app_init_services() {
    touch_init(PA_4); // TODO - we have 8 channel touch there
}
