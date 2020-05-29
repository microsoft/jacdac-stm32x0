#include "jdprofile.h"

DEVICE_CLASS(0x32f59e1b, "JM SND v2.0");

void init_services() {
    snd_init(PA_4);
}
