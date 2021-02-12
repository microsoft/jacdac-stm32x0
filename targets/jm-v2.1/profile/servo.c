#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x3ac74e57, "JM PWM+SERVO v2.1");

const servo_params_t servo_params = {
    .pin = PA_7,
    .fixed = 0,
    .variant = 1, // JD_SERVO_VARIANT_POSITIONAL_ROTATION
    .min_angle = -90 << 16,
    .min_pulse = 500,
    .max_angle = 90 << 16,
    .max_pulse = 2500,
};

void app_init_services() {
#if 1
    pin_setup_output(PF_1);
    pin_set(PF_1, 0); // enable JD power
#endif

    servo_init(&servo_params);

    // The sense lines are not connected on this hw revision, so we just skip the actual power
    // service.
}
