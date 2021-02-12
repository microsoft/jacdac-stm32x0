#include "jdprofile.h"

FIRMWARE_IDENTIFIER(0x30b0c24e, "JM PWM (Servo) v2.0");


const servo_params_t servo_params = {
    .pin = PA_7,
    .fixed = 0,
    .variant = 1,
    .min_angle = -90 << 16,
    .min_pulse = 500,
    .max_angle = 90 << 16,
    .max_pulse = 2500,
};

void app_init_services() {
    servo_init(&servo_params);
}
