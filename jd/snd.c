#include "jdsimple.h"

#define CMD_PLAY 0x80

#ifndef SND_OFF
#define SND_OFF 1
#endif

#define SW_TRIANGLE 1
#define SW_SAWTOOTH 2
#define SW_SINE 3 // TODO remove it? it takes space
#define SW_NOISE 5
#define SW_SQUARE_10 11
#define SW_SQUARE_50 15

// for now we approximate the actual synth
typedef struct {
    uint8_t soundWave;
    uint8_t flags;
    uint16_t frequency;    // Hz
    uint16_t duration;     // ms
    uint16_t startVolume;  // 0-1023
    uint16_t endVolume;    // 0-1023
    uint16_t endFrequency; // Hz
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

REG_DEFINITION(                //
    snd_regs,                  //
    REG_SRV_BASE,              //
    REG_U16(JD_REG_INTENSITY), //
)

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;
    if (on) {
        pwr_enter_tim();
    } else {
        pin_set(state->pin, SND_OFF);
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
    uint32_t period = 1000000 / ((t->frequency + t->endFrequency) >> 1);
    if (period > 0xfff0)
        period = 0xfff0;
    uint32_t vol = (t->startVolume + t->endVolume) >> 1;
    if (vol > 1023)
        vol = 1023;
    vol = (vol * state->volume) >> 11;
    uint32_t duty = (t->period * vol) >> 11;
#if SND_OFF == 1
    duty = t->period - duty;
#endif
    state->pwm_pin = pwm_init(state->pin, t->period, duty, cpu_mhz);
}

void snd_handle_packet(srv_t *state, jd_packet_t *pkt) {
    srv_handle_reg(state, pkt, snd_regs);
    if (state->volume > 1023)
        state->volume = 1023;
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

    state->volume = 1023;

    state->next_tone_time = now;
    state->num_tones = 1;
    state->tones[0].period = 2500;
    state->tones[0].duration = 500;
}
