#include "jdsimple.h"

#define EVT_DOWN 1
#define EVT_UP 2
#define EVT_CLICK 3
#define EVT_LONG_CLICK 4

struct srv_state {
    SENSOR_COMMON;
    uint8_t pin;
    uint8_t pressed;
    uint8_t prev_pressed;
    uint32_t press_time;
    uint32_t nextSample;
};

static void emit_event(srv_t *state, int ev) {
    if (ev & ~0xff)
        jd_panic();
    txq_push(state->service_number, JD_CMD_EVENT, &ev, 4);
}

static void update(srv_t *state) {
    state->pressed = !pin_get(state->pin);
    if (state->pressed != state->prev_pressed) {
        state->prev_pressed = state->pressed;
        if (state->pressed) {
            emit_event(state, EVT_DOWN);
            state->press_time = now;
        } else {
            emit_event(state, EVT_UP);
            uint32_t presslen = now - state->press_time;
            if (presslen > 500000)
                emit_event(state, EVT_LONG_CLICK);
            else
                emit_event(state, EVT_CLICK);
        }
    }
}

void btn_process(srv_t *state) {
    if (should_sample(&state->nextSample, 9000)) {
        update(state);

        if (sensor_should_stream(state) && state->pressed)
            txq_push(state->service_number, JD_CMD_GET_REG | JD_REG_READING, &state->pressed,
                     sizeof(state->pressed));
    }
}

void btn_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->pressed, sizeof(state->pressed));
}

SRV_DEF(btn, JD_SERVICE_CLASS_BUTTON);

void btn_init(uint8_t pin) {
    SRV_ALLOC(btn);
    state->pin = pin;
    pin_setup_input(state->pin, 0);
    update(state);
}
