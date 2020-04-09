#include "jdsimple.h"

static void identify(int num) {
    static uint8_t id_counter;
    static uint32_t nextblink;
    if (num) {
        id_counter = num;
        nextblink = now;
    }
    if (!id_counter)
        return;
    if (!should_sample(&nextblink, 150000))
        return;

    id_counter--;
    led_blink(50000);
}

void ctrl_init(uint8_t service_num) {}

void ctrl_process(void) {
    identify(0);
}

void ctrl_handle_pkt(jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_CMD_ADVERTISEMENT_DATA:
        app_queue_annouce();
        break;
    case JD_CMD_CTRL_IDENTIFY:
        identify(7);
        break;
    case JD_CMD_CTRL_RESET:
        NVIC_SystemReset();
        break;
    }
}

const host_service_t host_ctrl = {
    .service_class = JD_SERVICE_CLASS_CTRL,
    .init = ctrl_init,
    .process = ctrl_process,
    .handle_pkt = ctrl_handle_pkt,
};
