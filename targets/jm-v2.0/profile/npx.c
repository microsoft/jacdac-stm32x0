#include "jdprofile.h"
#include "jacdac/dist/c/light.h"

FIRMWARE_IDENTIFIER(0x35643e91, "JM PWM (npx) v2.0");

void app_init_services() {
    light_init(JD_LIGHT_LIGHT_TYPE_WS2812B_GRB, 15, 200);
}
