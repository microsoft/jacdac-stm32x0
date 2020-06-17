#include "jdsimple.h"

#define PWM_BITS 9
#define SERVO_PERIOD (1 << PWM_BITS)
#define LOG NOLOG

struct srv_state {
    SRV_COMMON;
    int16_t value;
    uint8_t intensity;
    uint8_t pwm_pin1;
    uint8_t pwm_pin2;
    uint8_t pin1;
    uint8_t pin2;
    uint8_t pin_en;
    uint8_t is_on;
};

REG_DEFINITION(               //
    motor_regs,               //
    REG_SRV_BASE,             //
    REG_S16(JD_REG_VALUE),    //
    REG_U8(JD_REG_INTENSITY), //
)

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;

    LOG("PWR %d", on);
    pin_set(state->pin1, 0);
    pin_set(state->pin2, 0);

    if (on) {
        pwr_enter_tim();
        if (!state->pwm_pin1) {
            state->pwm_pin1 = pwm_init(state->pin1, SERVO_PERIOD, 0, cpu_mhz / 16);
            state->pwm_pin2 = pwm_init(state->pin2, SERVO_PERIOD, 0, cpu_mhz / 16);
        }
        pin_set(state->pin_en, 1);
        pwm_enable(state->pwm_pin1, 1);
        pwm_enable(state->pwm_pin2, 1);
    } else {
        pin_set(state->pin_en, 0);
        pwm_set_duty(state->pwm_pin1, 0);
        pwm_set_duty(state->pwm_pin2, 0);
        pwm_enable(state->pwm_pin1, 0);
        pwm_enable(state->pwm_pin2, 0);
        pwr_leave_tim();
    }
    state->is_on = on;
    LOG("PWR OK");
}

void motor_process(srv_t *state) {}

static int pwm_value(int v) {
    v >>= 15 - PWM_BITS;
    if (v >= (1 << PWM_BITS))
        v = (1 << PWM_BITS) - 1;
    return v;
}

void motor_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (srv_handle_reg(state, pkt, motor_regs)) {
        set_pwr(state, !!state->intensity);
        if (state->is_on) {
            LOG("PWM set %d", state->value);
            if (state->value < 0) {
                pwm_set_duty(state->pwm_pin1, 0);
                pwm_set_duty(state->pwm_pin2, pwm_value(-state->value));
                LOG("PWM set2 %d", pwm_value(-state->value));
            } else {
                pwm_set_duty(state->pwm_pin2, 0);
                pwm_set_duty(state->pwm_pin1, pwm_value(state->value));
                LOG("PWM set1 %d", pwm_value(state->value));
            }
        }
    }
}

SRV_DEF(motor, JD_SERVICE_CLASS_MOTOR);
void motor_init(uint8_t pin1, uint8_t pin2, uint8_t pin_nsleep) {
    SRV_ALLOC(motor);
    state->pin1 = pin1;
    state->pin2 = pin2;
    state->pin_en = pin_nsleep;
    pin_setup_output(state->pin_en);
    pin_setup_output(state->pin1);
    pin_setup_output(state->pin2);
}
