#include "lib.h"

#ifdef BRIDGEQ

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;

    uint8_t pin_cs, pin_txrq;
    uint8_t rx_size, shift, skip_one;
    queue_t rx_q;
    uint32_t next_send;
    jd_frame_t spi_rx;
};

REG_DEFINITION(               //
    bridge_regs,              //
    REG_SRV_BASE,             //
    REG_U8(JD_REG_INTENSITY), //
)

void jd_send_low(jd_frame_t *f);

static srv_t *_state;

static void spi_done_handler(void) {
    srv_t *state = _state;
    unsigned frmsz = JD_FRAME_SIZE(&state->spi_rx);

    if (state->spi_rx.size == 0xFE) {
        // m:b didn't manage to read the packet; try again
        state->rx_size = 0;
        pin_set(state->pin_cs, 1);
        pwr_leave_tim();
        state->next_send = now;
        tim_max_sleep = 100;
        return;
    }

    if (frmsz > state->rx_size && state->spi_rx.size != 0xFF) {
        // we didn't read enough
        uint8_t *d = (uint8_t *)&state->spi_rx + state->rx_size;
        unsigned left = frmsz - state->rx_size;
        state->rx_size = frmsz;
        dspi_xfer(NULL, d, left, spi_done_handler);
    } else {
        state->rx_size = 0;
        pin_set(state->pin_cs, 1);
        pwr_leave_tim();

        if (state->shift)
            queue_shift(state->rx_q);

        if (4 <= state->spi_rx.size && state->spi_rx.size <= 240) {
            jd_send_low(&state->spi_rx);
            // also process packets ourselves - m:b might be talking to us
            jd_services_process_frame(&state->spi_rx);
        }

        state->next_send = now + 99;
        tim_max_sleep = 100;
    }
}

static void xchg(srv_t *state) {
    if (state->rx_size)
        return;

    if (in_future(state->next_send)) {
        tim_max_sleep = 100;
        return;
    }

    if (state->skip_one) {
        state->skip_one = 0;
        return;
    }

    // tim_max_sleep = 0; // TODO work on power consumption
    jd_frame_t *fwd = queue_front(state->rx_q);
    int size = 32;
    if (fwd && JD_FRAME_SIZE(fwd) > size)
        size = JD_FRAME_SIZE(fwd);
    state->rx_size = size;
    state->shift = !!fwd;
    pin_set(state->pin_cs, 0);
    pwr_enter_tim();
    *(uint32_t *)&state->spi_rx = 0;
    dspi_xfer(fwd, &state->spi_rx, size, spi_done_handler);
}

void bridge_forward_frame(jd_frame_t *frame) {
    if (frame != &_state->spi_rx)
        queue_push(_state->rx_q, frame);
}

void bridge_process(srv_t *state) {
    if (queue_front(state->rx_q) || (!in_future(state->next_send) && pin_get(state->pin_txrq) == 0))
        xchg(state);
}

void bridge_handle_packet(srv_t *state, jd_packet_t *pkt) {
    service_handle_register(state, pkt, bridge_regs);
}

SRV_DEF(bridge, JD_SERVICE_CLASS_BRIDGE);
void bridge_init(uint8_t pin_cs, uint8_t pin_txrq) {
    SRV_ALLOC(bridge);
    dspi_init();
    state->pin_cs = pin_cs;
    pin_set(pin_cs, 1);
    pin_setup_output(pin_cs);
    state->pin_txrq = pin_txrq;
    pin_setup_input(pin_txrq, 1);
    state->rx_q = queue_alloc(512);
    _state = state;
}

// alternative tx_Q impl.
static queue_t tx_q;
static bool isSending;

int jd_tx_is_idle() {
    return !isSending && queue_front(tx_q) == NULL;
}

void jd_tx_init(void) {
    tx_q = queue_alloc(512);
}

void jd_send_low(jd_frame_t *f) {
    jd_compute_crc(f);
    queue_push(tx_q, f);
    jd_packet_ready();
}

void jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size) {
    if (service_size > 32)
        jd_panic();

    uint32_t buf[(service_size + 16 + 3) / 4];
    jd_frame_t *f = (jd_frame_t *)buf;

    jd_reset_frame(f);
    void *trg = jd_push_in_frame(f, service_num, service_cmd, service_size);
    memcpy(trg, data, service_size);

    f->device_identifier = jd_device_id();
    jd_send_low(f);
    if (_state) {
        queue_push(_state->rx_q, f); // also forward packets we generate ourselves
        _state->skip_one = 1;
    }
}

void jd_send_event_ext(srv_t *srv, uint32_t eventid, uint32_t arg) {
    srv_common_t *state = (srv_common_t *)srv;
    if (eventid >> 16)
        jd_panic();
    uint32_t data[] = {eventid, arg};
    jd_send(state->service_number, JD_CMD_EVENT, data, 8);
}

// bridge between phys and queue imp, phys calls this to get the next frame.
jd_frame_t *jd_tx_get_frame(void) {
    jd_frame_t *f = queue_front(tx_q);
    if (f)
        isSending = true;
    return f;
}

// bridge between phys and queue imp, marks as sent.
void jd_tx_frame_sent(jd_frame_t *pkt) {
    isSending = false;
    queue_shift(tx_q);
    if (queue_front(tx_q))
        jd_packet_ready(); // there's more to do
}

void jd_tx_flush() {
    // do nothing
}

#endif