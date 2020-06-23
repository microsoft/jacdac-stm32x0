#include "jdprofile.h"

DEVICE_CLASS(0x39a9dc81, "JDF030 touch v0");

void app_init_services() {
    touch_init(PA_4);
}
