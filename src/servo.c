#include "jdsimple.h"

#define SERVO_PERIOD 20000

struct srv_state {
    SRV_COMMON;
    uint32_t pulse;
    uint8_t intensity;
    uint8_t pwm_pin;
    uint8_t is_on;
};

REG_DEFINITION(               //
    servo_regs,               //
    REG_SRV,                  //
    REG_U32(JD_REG_VALUE),    //
    REG_U8(JD_REG_INTENSITY), //
)

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;
    if (on) {
        pwr_enter_pll();
        if (!state->pwm_pin)
            state->pwm_pin = pwm_init(PIN_SERVO, SERVO_PERIOD, 0, 8);
        pwm_enable(state->pwm_pin, 1);
    } else {
        pin_set(PIN_SERVO, 0);
        pwm_enable(state->pwm_pin, 0);
        pwr_leave_pll();
    }
    pin_set(PIN_PWR, !on);
    state->is_on = on;
}


void servo_process(srv_t *state) {}

void servo_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (srv_handle_reg(state, pkt, servo_regs)) {
        set_pwr(state, !!state->intensity);
        if (state->is_on)
            pwm_set_duty(state->pwm_pin, state->pulse);
    }
}

SRV_DEF(servo, JD_SERVICE_CLASS_SERVO);
void servo_init() {
    SRV_ALLOC(servo);
}
