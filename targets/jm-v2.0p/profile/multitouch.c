#include "jdprofile.h"

DEVICE_CLASS(0x3f9bc26a, "JM Touch-Proto v2.0");

static const uint8_t touchpins1[] = {PA_0, PA_1, PA_3, PA_4, -1};
void app_init_services() {
    multitouch_init(touchpins1);
}
