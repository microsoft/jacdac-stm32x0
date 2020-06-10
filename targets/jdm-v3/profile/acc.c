#include "jdprofile.h"

DEVICE_CLASS(0x334b4fb6, "JDM3 acc");

void app_init_services() {
    acc_init();
}
