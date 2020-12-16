#include "jdprofile.h"

// this modules doesn't exists but we want to try a test build of TH02 sensor
FIRMWARE_IDENTIFIER(0x37469e2f, "JM Env v2.0");

void app_init_services() {
    temp_init();
    humidity_init();
}
