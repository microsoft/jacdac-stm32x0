#include "jdsimple.h"

// do not use _state parameter in this file - it can be NULL in bootloader mode

struct srv_state {
    SRV_COMMON;
};

static uint8_t id_counter;
static uint32_t nextblink;

static void identify(void) {
    if (!id_counter)
        return;
    if (!should_sample(&nextblink, 150000))
        return;

    id_counter--;
    led_blink(50000);
}

void ctrl_process(srv_t *_state) {
    identify();
}

void ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_CMD_ADVERTISEMENT_DATA:
        app_queue_annouce();
        break;
    case JD_CMD_CTRL_IDENTIFY:
        id_counter = 7;
        nextblink = now;
        identify();
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
