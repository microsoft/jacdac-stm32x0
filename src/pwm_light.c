#include "jdsimple.h"

#define DEFAULT_MAXPOWER 100

#define LOG DMESG

#define PWM_REG_CURR_ITERATION 0x80
#define PWM_REG_MAX_ITERATIONS 0x81
#define PWM_REG_STEPS 0x82
#define PWM_REG_MAX_STEPS 0x180

#define MAX_STEPS 10
#define UPDATE_US 10000
#define PWM_PERIOD_BITS 13 // at 12 bit you can still see the steps at the low end
#define PWM_PERIOD (1 << PWM_PERIOD_BITS)

typedef struct {
    uint16_t start_intensity;
    uint16_t duration; // in ms
} __attribute__((aligned(4))) step_t;

struct pwm_light_state {
    uint16_t intensity;
    uint16_t maxpower;
    uint16_t curr_iteration;
    uint16_t max_iterations;
    uint8_t max_steps;
    uint8_t pwm_pin;
    step_t steps[MAX_STEPS];
    // internal state;
    uint32_t step_start_time;
    uint32_t nextFrame;
};

REG_DEFINITION(                                           //
    pwm_light_regs,                                       //
    REG_U16(JD_REG_INTENSITY),                            //
    REG_U16(JD_REG_MAX_POWER),                            //
    REG_U16(PWM_REG_CURR_ITERATION),                      //
    REG_U16(PWM_REG_MAX_ITERATIONS),                      //
    REG_U8(PWM_REG_MAX_STEPS),                            //
    REG_U8(JD_REG_PADDING),                               // pwm_pin not accessible
    REG_BYTES(PWM_REG_STEPS, MAX_STEPS * sizeof(step_t)), //
)

static const uint16_t glow[] = {0xffff, 1500, 0x0f00, 1500, 0xffff, 0};
static const uint16_t stable[] = {0xffff, 10000, 0xffff, 0};

static struct pwm_light_state state;

void pwm_light_init(uint8_t service_num) {
    state.max_steps = MAX_STEPS;

    memcpy(state.steps, stable, sizeof(stable));
    memcpy(state.steps, glow, sizeof(glow));

    state.max_iterations = 0xffff;
    state.intensity = 0;
    state.pwm_pin = pwm_init(PIN_GLO1, PWM_PERIOD, PWM_PERIOD - 1, 1);
}

void pwm_light_process() {
    if (!should_sample(&state.nextFrame, UPDATE_US))
        return;

    int step_intensity = -1;

    while (state.curr_iteration <= state.max_iterations) {
        uint32_t tm = tim_get_micros() >> 10;
        uint32_t delta = tm - state.step_start_time;
        uint32_t pos = 0;
        step_t *st = &state.steps[0];
        for (int i = 0; i < MAX_STEPS; ++i) {
            unsigned d = st[i].duration;
            if (d == 0)
                break;
            pos += d;
            int into = pos - delta;
            if (into >= 0) {
                int i0 = st[i].start_intensity;
                int i1 = st[i + 1].start_intensity;
                if (d == 0 || i0 == i1)
                    step_intensity = i0;
                else
                    step_intensity = (unsigned)(i0 * into + i1 * (d - into)) / d;
                break;
            }
        }
        if (step_intensity >= 0 || pos == 0)
            break;

        state.curr_iteration++;
        state.step_start_time = tm;
    }

    if (step_intensity < 0)
        step_intensity = 0;

    step_intensity = ((uint32_t)step_intensity * state.intensity) >> 16;
    int v = step_intensity >> (16 - PWM_PERIOD_BITS);
    pwm_set_duty(state.pwm_pin, PWM_PERIOD - v);
}

void pwm_light_handle_packet(jd_packet_t *pkt) {
    switch (handle_reg(&state, pkt, pwm_light_regs)) {
    case PWM_REG_STEPS:
        state.curr_iteration = 0;
        state.step_start_time = tim_get_micros() >> 10;
        break;
    }
}

const host_service_t host_pwm_light = {
    .service_class = JD_SERVICE_CLASS_PWM_LIGHT,
    .init = pwm_light_init,
    .process = pwm_light_process,
    .handle_pkt = pwm_light_handle_packet,
};
