#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x36d257aa, "JM D-Pad v2.0");

// left, up, right, down, a, b, menu, menu2, reset, exit
static const uint8_t btnPins[] = {PA_2, PA_5, PA_4, PA_3, PA_6, PA_7, PB_1, -1, PA_10};

void app_init_services() {
    gamepad_init(sizeof(btnPins), btnPins, NULL);
}
