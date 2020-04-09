#include "jdsimple.h"

static sensor_state_t sensor;
static int32_t sample, position;
static uint8_t state, inited;
static uint32_t nextSample;

void crank_init(uint8_t service_num) {
    sensor.service_number = service_num;
}

const static int8_t posMap[] = {0, +1, -1, +2, -1, 0, -2, +1, +1, -2, 0, -1, +2, -1, +1, 0};
static void update(void) {
    // based on comments in https://github.com/PaulStoffregen/Encoder/blob/master/Encoder.h
    uint16_t s = state & 3;
    if (pin_get(PIN_P0))
        s |= 4;
    if (pin_get(PIN_P1))
        s |= 8;

    state = (s >> 2);
    if (posMap[s]) {
        position += posMap[s];
        sample = position >> 2;
    }
}

static void maybe_init() {
    if (sensor.is_streaming && !inited) {
        inited = true;
        pin_setup_input(PIN_P0, 1);
        pin_setup_input(PIN_P1, 1);
        update();
    }
}

void crank_process() {
    maybe_init();

    if (should_sample(&nextSample, 997) && inited)
        update();

#if 0
    static int last;
    if (sample != last) {
        DMESG("s:%d", sample);
        pin_set(PIN_SERVO, 1);
        pin_set(PIN_SERVO, 0);
    }
    last = sample;
#endif

    if (sensor_should_stream(&sensor))
        txq_push(sensor.service_number, JD_CMD_GET_REG | JD_REG_READING, &sample, sizeof(sample));
}

void crank_handle_packet(jd_packet_t *pkt) {
    sensor_handle_packet(&sensor, pkt);

    if (pkt->service_command == (JD_CMD_GET_REG | JD_REG_READING))
        txq_push(pkt->service_number, pkt->service_command, &sample, sizeof(sample));
}

const host_service_t host_crank = {
    .service_class = JD_SERVICE_CLASS_ROTARY_ENCODER,
    .init = crank_init,
    .process = crank_process,
    .handle_pkt = crank_handle_packet,
};
