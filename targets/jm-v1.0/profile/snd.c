#include "jdprofile.h"

DEVICE_CLASS(0x3707f76a, "JM SND v1.0");

void app_init_services() {
    snd_init(PIN_PWR);
}
