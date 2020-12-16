#include "jdprofile.h"

// Edit the string below to match your company name, the device name, and hardware revision.
FIRMWARE_IDENTIFIER(0x3dfafd42, "Example Corp. Servo Rev.A");

void app_init_services() {
    // see jacdac-c/services/interfaces/jd_service_initializers.h for the services that can be enabled here
    // you can enable zero or more services
    servo_init(PA_7);
}
