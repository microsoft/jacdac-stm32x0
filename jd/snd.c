#include "jdsimple.h"

#define CMD_PLAY 0x80

typedef struct {
    uint16_t period;   // us
    uint16_t duration; // ms
} tone_t;

struct srv_state {
    SRV_COMMON;
    uint8_t volume;
    uint8_t pwm_pin;
    uint8_t is_on;
    uint8_t pin;
    uint8_t tone_ptr;
    uint8_t num_tones;
    uint32_t next_tone_time;
    tone_t tones[JD_SERIAL_PAYLOAD_SIZE / sizeof(tone_t)];
};

REG_DEFINITION(               //
    snd_regs,                 //
    REG_SRV_BASE,             //
    REG_U8(JD_REG_INTENSITY), //
)

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;
    if (on) {
        pwr_enter_tim();
    } else {
        pin_set(state->pin, 1);
        pwm_enable(state->pwm_pin, 0);
        pwr_leave_tim();
    }
    state->is_on = on;
}

void snd_process(srv_t *state) {
    if (state->num_tones == 0) {
        set_pwr(state, 0);
        return;
    }

    if (in_future(state->next_tone_time))
        return;

    if (state->tone_ptr >= state->num_tones) {
        set_pwr(state, 0);
        return;
    }

    tone_t *t = &state->tones[state->tone_ptr++];
    state->next_tone_time += t->duration * 1000;
    set_pwr(state, 1);
    state->pwm_pin =
        pwm_init(state->pin, t->period, t->period - ((t->period * state->volume) >> 9), cpu_mhz);
}

void snd_handle_packet(srv_t *state, jd_packet_t *pkt) {
    srv_handle_reg(state, pkt, snd_regs);
    switch (pkt->service_command) {
    case CMD_PLAY:
        state->next_tone_time = now;
        state->tone_ptr = 0;
        state->num_tones = pkt->service_size / sizeof(tone_t);
        memcpy(state->tones, pkt->data, pkt->service_size);
        snd_process(state);
        break;
    }
}

SRV_DEF(snd, JD_SERVICE_CLASS_MUSIC);
void snd_init(uint8_t pin) {
    SRV_ALLOC(snd);
    state->pin = pin;

    state->volume = 255;

    state->next_tone_time = now;
    state->num_tones = 1;
    state->tones[0].period = 2500;
    state->tones[0].duration = 5000;
}
