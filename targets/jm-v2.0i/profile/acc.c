#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x3ccea1a9, "JM Acc v2.0");

// this depends on physical placement of accelerometer on PCB
void acc_data_transform(int32_t sample[3]) {
    int32_t v0 = sample[0];
    int32_t v1 = sample[1];
    int32_t v2 = sample[2];
    sample[0] = v1;
    sample[1] = -v0;
    sample[2] = -v2;
}

void app_init_services() {
    acc_init();
}
