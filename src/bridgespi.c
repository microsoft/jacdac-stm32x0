#include "lib.h"

// SPI-Jacdac bridge for RPi

#ifdef PIN_BR_TX_READY

#if !JD_BRIDGE || !JD_SEND_FRAME
#error "JD_BRIDGE reqd"
#endif

#define XFER_SIZE 256
#define TXQ_SIZE 1024
#define RXQ_SIZE 1024

#define JD_SERVICE_CLASS_BRIDGE 0x1fe5b46f

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;

    uint16_t rx_dst; // points to spi_bridge (or past it)

    uint32_t connected_blink;
    uint32_t connected_blink_end;

    // uint8_t rx_size, shift, skip_one;
    jd_queue_t rx_q;

    uint8_t spi_host[XFER_SIZE];   // host sends it
    uint8_t spi_bridge[XFER_SIZE]; // we send it
};

REG_DEFINITION(               //
    bridge_regs,              //
    REG_SRV_COMMON,           //
    REG_U8(JD_REG_INTENSITY), //
)

static srv_t *_state;

static inline bool is_host_frame(jd_frame_t *f) {
    return _state->spi_host <= (uint8_t *)f && (uint8_t *)f <= &_state->spi_host[XFER_SIZE];
}

static void setup_xfer(srv_t *state);
static void spi_done_handler(void) {
    srv_t *state = _state;

    if (jd_should_sample(&state->connected_blink, 2 * 512 * 1024)) {
        // jd_status_set_ch(2, 150);
        state->connected_blink_end = tim_get_micros() + 50 * 1024;
    }

    jd_frame_t *frame = (jd_frame_t *)state->spi_host;

    while (frame->size && (uint8_t *)frame < &_state->spi_host[XFER_SIZE]) {
        unsigned frmsz = JD_FRAME_SIZE(frame);
        jd_send_frame_raw(frame);
        frame = (jd_frame_t *)((uint8_t *)frame + ((frmsz + 3) & ~3));
    }

    setup_xfer(state);
}

static void setup_xfer(srv_t *state) {
    int has_rx = 0;
    int dst = 0;
    jd_frame_t *frame = NULL;
    while (dst < XFER_SIZE) {
        frame = jd_queue_front(state->rx_q);
        if (!frame)
            break;
        int sz = JD_FRAME_SIZE(frame);
        sz = (sz + 3) & ~3;
        if (dst + sz > XFER_SIZE)
            break;
        memcpy(&state->spi_bridge[dst], frame, sz);
        dst += sz;
        jd_queue_shift(state->rx_q);
        has_rx = 1;
    }

    if (!frame && dst < 16) {
        // if we didn't put enough at the beginning of spi_bridge[], put in a fake packet
        memset(&state->spi_bridge[dst], 0xff, 16);
        state->spi_bridge[dst + 2] = 4; // frame data size = 4 -> frame size = 16
        dst += 16;
    }

    if (dst < XFER_SIZE)
        *(uint32_t *)&state->spi_bridge[dst] = 0;

    target_disable_irq();
    if (frame)
        state->rx_dst = XFER_SIZE;
    else
        state->rx_dst = dst;

    state->spi_host[2] = 0xff;
    spis_xfer(state->spi_bridge, state->spi_host, XFER_SIZE, spi_done_handler);

    pin_set(PIN_BR_TX_READY, jd_tx_will_fit(XFER_SIZE));
    pin_set(PIN_BR_RX_READY, has_rx);
    target_enable_irq();
}

// called when physical layer received a frame
void bridge_forward_frame(jd_frame_t *frame) {
    if (is_host_frame(frame))
        JD_PANIC(); // loop?

    srv_t *state = _state;

    if (!state->enabled)
        return;

    int sz = JD_FRAME_SIZE(frame);

    target_disable_irq();
    if (state->rx_dst + sz > XFER_SIZE || state->spi_host[2] != 0xff) {
        state->rx_dst = XFER_SIZE;
        jd_queue_push(state->rx_q, frame);
    } else {
        sz = (sz + 3) & ~3;
        memcpy(&state->spi_bridge[state->rx_dst], frame, sz);
        state->rx_dst += sz;
        if (state->rx_dst < XFER_SIZE)
            *(uint32_t *)&state->spi_bridge[state->rx_dst] = 0;
        pin_set(PIN_BR_RX_READY, 1);
    }
    target_enable_irq();
}

void bridge_process(srv_t *state) {
    if (state->connected_blink_end && in_past(state->connected_blink_end)) {
        state->connected_blink_end = 0;
        // jd_status_set_ch(2, 0);
    }
}

void bridge_handle_packet(srv_t *state, jd_packet_t *pkt) {
    service_handle_register_final(state, pkt, bridge_regs);
}

static void halftransfer(void) {
    pin_set(PIN_BR_RX_READY, 0);
    pin_set(PIN_BR_TX_READY, 0);
}

SRV_DEF(bridge, JD_SERVICE_CLASS_BRIDGE);
void bridge_init(void) {
    SRV_ALLOC(bridge);

    pwr_enter_pll(); // full speed ahead!

    spis_init();
    spis_halftransfer_cb = halftransfer;

    pin_setup_output(PIN_BR_TX_READY);
    pin_setup_output(PIN_BR_RX_READY);

    state->rx_q = jd_queue_alloc(RXQ_SIZE);
    state->enabled = 1;
    _state = state;

    setup_xfer(state);
}

#endif