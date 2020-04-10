#include "jdsimple.h"

#define LOG DMESG

#define SERVO_PERIOD 20000

struct servo_state {
    uint32_t pulse;
    uint8_t intensity;
    uint8_t pwm_pin;
};

static struct servo_state state;

REG_DEFINITION(               //
    servo_regs,               //
    REG_U32(JD_REG_VALUE),    //
    REG_U8(JD_REG_INTENSITY), //
)

void servo_init(uint8_t service_num) {}

void servo_process() {}

void servo_handle_packet(jd_packet_t *pkt) {
    if (handle_reg(&state, pkt, servo_regs)) {
        if (!state.pwm_pin) {
            state.pwm_pin = pwm_init(PIN_SERVO, SERVO_PERIOD, 0, 8);
        }
        pin_set(PIN_PWR, !state.intensity);
        pwm_set_duty(state.pwm_pin, state.pulse);
    }
}

const host_service_t host_servo = {
    .service_class = JD_SERVICE_CLASS_SERVO,
    .init = servo_init,
    .process = servo_process,
    .handle_pkt = servo_handle_packet,
};
