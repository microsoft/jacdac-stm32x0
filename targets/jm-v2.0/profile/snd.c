#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x32f59e1b, "JM SND v2.0");

void app_init_services() {
    buzzer_init(PA_4);
}
