#include "jdsimple.h"

const host_service_t *services[] = {
    &host_ctrl,
#ifndef NO_ACC
    &host_accelerometer,
#endif
    &host_light,         &host_pwm_light, &host_crank,
};

#define NUM_SERVICES (sizeof(services) / sizeof(services[0]))

uint32_t now;
static uint64_t maxId;
static uint32_t lastMax, lastDisconnectBlink;

void app_queue_annouce() {
    alloc_stack_check();

    uint32_t *dst =
        txq_push(JD_SERVICE_NUMBER_CTRL, JD_CMD_ADVERTISEMENT_DATA, NULL, NUM_SERVICES * 4);
    if (!dst)
        return;
    for (int i = 0; i < NUM_SERVICES; ++i)
        dst[i] = services[i]->service_class;

#ifdef JDM_V2
    pin_setup_output(PIN_P0);
    pin_setup_output(PIN_SERVO);
    pin_set(PIN_P0, 1);
    pin_set(PIN_SERVO, 1);
    pin_set(PIN_SERVO, 0);
    pin_set(PIN_PWR, 0);
    pin_set(PIN_PWR, 1);
    pin_set(PIN_SERVO, 1);
    pin_set(PIN_SERVO, 0);
    pin_set(PIN_P0, 0);
#endif
}

#ifdef CNT_FLOOD
static uint32_t cnt_count;
static uint32_t prevCnt;
uint32_t numErrors, numPkts;
#endif

void app_init_services() {
    for (int i = 0; i < NUM_SERVICES; ++i) {
        services[i]->init(i);
    }
}

void app_process() {
#ifdef CNT_FLOOD
    if (txq_is_idle()) {
        cnt_count++;
        txq_push(0x42, 0x80, 0, &cnt_count, sizeof(cnt_count));
    }
#endif

    if (should_sample(&lastDisconnectBlink, 250000)) {
        if (in_past(lastMax + 2000000)) {
            led_blink(5000);
        }
    }

    for (int i = 0; i < NUM_SERVICES; ++i) {
        services[i]->process();
    }

    txq_flush();
}

static void handle_ctrl_tick(jd_packet_t *pkt) {
    if (pkt->service_command == JD_CMD_ADVERTISEMENT_DATA) {
        // if we have not seen maxId for 1.1s, find a new maxId
        if (pkt->device_identifier < maxId && in_past(lastMax + 1100000)) {
            maxId = pkt->device_identifier;
        }

        // maxId? blink!
        if (pkt->device_identifier >= maxId) {
            maxId = pkt->device_identifier;
            lastMax = now;
            led_blink(50);
        }
    }
}

static void handle_packet(jd_packet_t *pkt) {
#ifdef CNT_FLOOD
    if (pkt->service_number == 0x42) {
        numPkts++;
        uint32_t c;
        memcpy(&c, pkt->data, sizeof(c));
        if (prevCnt && prevCnt + 1 != c) {
            log_pin_set(2, 1);
            log_pin_set(2, 0);
            numErrors++;
            DMESG("ERR %d/%d %d", numErrors, numPkts, numErrors * 10000 / numPkts);
        }
        prevCnt = c;
    }
#endif

    if (!(pkt->flags & JD_FRAME_FLAG_COMMAND)) {
        if (pkt->service_number == 0)
            handle_ctrl_tick(pkt);
        return;
    }

    bool matched_devid = pkt->device_identifier == device_id();

    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        for (int i = 0; i < NUM_SERVICES; ++i) {
            if (pkt->device_identifier == services[i]->service_class) {
                pkt->service_number = i;
                matched_devid = true;
                break;
            }
        }
    }

    if (!matched_devid)
        return;

    // dump_pkt(pkt, "TOP");

    if (pkt->service_number < NUM_SERVICES)
        services[pkt->service_number]->handle_pkt(pkt);
}

int app_handle_frame(jd_frame_t *frame) {
    now = tim_get_micros();

    if (frame->flags & JD_FRAME_FLAG_ACK_REQUESTED && frame->flags & JD_FRAME_FLAG_COMMAND &&
        frame->device_identifier == device_id())
        txq_push(JD_SERVICE_NUMBER_CRC_ACK, frame->crc, NULL, 0);

    for (;;) {
        handle_packet((jd_packet_t *)frame);
        if (!jd_shift_frame(frame))
            break;
    }

    return 0;
}
