#include "jdprofile.h"

// Edit the string below to match your company name, the device name, and hardware revision.
// The 0x0 will be replaced with a unique identifier the first time you run make.
// Do not change the 0x3.... value, as that would break the firmware update process.
FIRMWARE_IDENTIFIER(0x0, "Example Corp. Servo Rev.A");

const servo_params_t servo_params = {
    .pin = PA_7,
    .fixed = 0, // just a controller, can modify min/max
    .variant = 1, // JD_SERVO_VARIANT_POSITIONAL_ROTATION
    .min_angle = -90 << 16, // -90.000deg
    .min_pulse = 500,       // 500us
    .max_angle = 90 << 16,  // +90.000deg
    .max_pulse = 2500,      // 2500us
};

void app_init_services() {
    // see jacdac-c/services/interfaces/jd_service_initializers.h for the services that can be
    // enabled here you can enable zero or more services
    servo_init(&servo_params);
}
