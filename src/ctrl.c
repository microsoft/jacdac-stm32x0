#include "jdsimple.h"

struct srv_state {
    SRV_COMMON;
    uint8_t id_counter;
    uint32_t nextblink;
};

static void identify(srv_t *state) {
    if (!state->id_counter)
        return;
    if (!should_sample(&state->nextblink, 150000))
        return;

    state->id_counter--;
    led_blink(50000);
}

void ctrl_process(srv_t *state) {
    identify(state);
}

void ctrl_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_CMD_ADVERTISEMENT_DATA:
        app_queue_annouce();
        break;
    case JD_CMD_CTRL_IDENTIFY:
        state->id_counter = 7;
        state->nextblink = now;
        identify(state);
        break;
    case JD_CMD_CTRL_RESET:
        target_reset();
        break;
    }
}

SRV_DEF(ctrl, JD_SERVICE_CLASS_CTRL);
void ctrl_init() {
    SRV_ALLOC(ctrl);
}
