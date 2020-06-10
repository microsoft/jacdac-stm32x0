#include "jdprofile.h"

DEVICE_CLASS(0x3479bec3, "JDM3 light");

void app_init_services() {
    light_init();
}
