#include "jdprofile.h"
#include "jacdac/dist/c/ledpixel.h"

FIRMWARE_IDENTIFIER(0x35643e91, "JM PWM (npx) v2.0");

void app_init_services() {
    ledpixel_init(JD_LED_PIXEL_LIGHT_TYPE_WS2812B_GRB, 15, 200);
}
