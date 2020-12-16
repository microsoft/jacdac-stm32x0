#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x3ccea1a9, "JM Acc v2.0");

void app_init_services() {
    acc_init();
}
