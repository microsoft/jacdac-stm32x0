#include "jdsimple.h"

#define LOG DMESG

#define SERVO_PERIOD 20000

struct servo_state {
    uint32_t pulse;
    uint8_t intensity;
    uint8_t pwm_pin;
    uint8_t is_on;
};

static struct servo_state state;

REG_DEFINITION(               //
    servo_regs,               //
    REG_U32(JD_REG_VALUE),    //
    REG_U8(JD_REG_INTENSITY), //
)

static void set_pwr(int on) {
    if (state.is_on == on)
        return;
    if (on) {
        pwr_enter_pll();
        if (!state.pwm_pin)
            state.pwm_pin = pwm_init(PIN_SERVO, SERVO_PERIOD, 0, 8);
        pwm_enable(state.pwm_pin, 1);
    } else {
        pin_set(PIN_SERVO, 0);
        pwm_enable(state.pwm_pin, 0);
        pwr_leave_pll();
    }
    pin_set(PIN_PWR, !on);
    state.is_on = on;
}

void servo_init(uint8_t service_num) {}

void servo_process() {}

void servo_handle_packet(jd_packet_t *pkt) {
    if (handle_reg(&state, pkt, servo_regs)) {
        set_pwr(!!state.intensity);
        if (state.is_on)
            pwm_set_duty(state.pwm_pin, state.pulse);
    }
}

const host_service_t host_servo = {
    .service_class = JD_SERVICE_CLASS_SERVO,
    .init = servo_init,
    .process = servo_process,
    .handle_pkt = servo_handle_packet,
};
