#include "jdprofile.h"
#include "jacdac/dist/c/ledpixel.h"

FIRMWARE_IDENTIFIER(0x3c6bebe4, "JM PWM+PWR (npx) v2.1");

void app_init_services() {
#if 0
    pin_setup_output(PF_1);
    pin_set(PF_1, 0); // enable JD power
#endif
    // The sense lines are not connected on this hw revision, so we just skip the actual power service.
    ledpixel_init(JD_LED_PIXEL_LIGHT_TYPE_WS2812B_GRB, 15, 200);
}
