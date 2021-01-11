#include "jdprofile.h"

// Edit the string below to match your company name, the device name, and hardware revision.
// The 0x0 will be replaced with a unique identifier the first time you run make.
// Do not change the 0x3.... value, as that would break the firmware update process.
FIRMWARE_IDENTIFIER(0x0, "Example Corp. Servo Rev.A");

void app_init_services() {
    // see jacdac-c/services/interfaces/jd_service_initializers.h for the services that can be enabled here
    // you can enable zero or more services
    servo_init(PA_7);
}
