#include "lib.h"

// SPI-Jacdac bridge for RPi

#ifdef BRIDGESPI

#define XFER_SIZE 256
#define TXQ_SIZE 1024
#define RXQ_SIZE 1024

static queue_t tx_q;

#define JD_SERVICE_CLASS_BRIDGE 0x1fe5b46f

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;

    uint16_t rx_dst; // points to spi_bridge (or past it)

    // uint8_t rx_size, shift, skip_one;
    queue_t rx_q;

    uint8_t spi_host[XFER_SIZE];   // host sends it
    uint8_t spi_bridge[XFER_SIZE]; // we send it
};

REG_DEFINITION(               //
    bridge_regs,              //
    REG_SRV_COMMON,           //
    REG_U8(JD_REG_INTENSITY), //
)

void jd_send_low(jd_frame_t *f);

static srv_t *_state;

static inline bool is_host_frame(jd_frame_t *f) {
    return _state->spi_host <= (uint8_t *)f && (uint8_t *)f <= &_state->spi_host[XFER_SIZE];
}

static void setup_xfer(srv_t *state);
static void spi_done_handler(void) {
    srv_t *state = _state;

    jd_frame_t *frame = (jd_frame_t *)state->spi_host;

    while (frame->size && (uint8_t *)frame < &_state->spi_host[XFER_SIZE]) {
        unsigned frmsz = JD_FRAME_SIZE(frame);
        jd_send_low(frame);
        // also process packets ourselves - RPi might be talking to us
        jd_services_process_frame(frame);
        frame = (jd_frame_t *)((uint8_t *)frame + ((frmsz + 3) & ~3));
    }

    setup_xfer(state);
}

static void setup_xfer(srv_t *state) {
    int has_rx = 0;
    int dst = 0;
    jd_frame_t *frame = NULL;
    while (dst < XFER_SIZE) {
        frame = queue_front(state->rx_q);
        if (!frame)
            break;
        int sz = JD_FRAME_SIZE(frame);
        sz = (sz + 3) & ~3;
        memcpy(&state->spi_bridge[dst], frame, sz);
        dst += sz;
        queue_shift(state->rx_q);
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

    pin_set(PIN_BR_TX_READY, queue_will_fit(tx_q, sizeof(XFER_SIZE)));
    pin_set(PIN_BR_RX_READY, has_rx);
    target_enable_irq();
}

// called when physical layer received a frame
void bridge_forward_frame(jd_frame_t *frame) {
    if (is_host_frame(frame))
        return; // ignore stuff we sent ourselves

    srv_t *state = _state;
    int sz = JD_FRAME_SIZE(frame);

    target_disable_irq();
    if (state->rx_dst + sz > XFER_SIZE || state->spi_host[2] != 0xff) {
        state->rx_dst = XFER_SIZE;
        queue_push(state->rx_q, frame);
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

void bridge_process(srv_t *state) {}

void bridge_handle_packet(srv_t *state, jd_packet_t *pkt) {
    service_handle_register(state, pkt, bridge_regs);
}

static void halftransfer(void) {
    pin_set(PIN_BR_RX_READY, 0);
    pin_set(PIN_BR_TX_READY, 0);
}

SRV_DEF(bridge, JD_SERVICE_CLASS_BRIDGE);
void bridge_init(uint8_t pin_cs, uint8_t pin_txrq) {
    SRV_ALLOC(bridge);

    pwr_enter_pll(); // full speed ahead!

    spis_init();
    spis_halftransfer_cb = halftransfer;

    pin_setup_output(PIN_BR_TX_READY);
    pin_setup_output(PIN_BR_RX_READY);

    state->rx_q = queue_alloc(RXQ_SIZE);
    _state = state;

    setup_xfer(state);
}

// alternative tx_Q impl.
static bool isSending;

int jd_tx_is_idle() {
    return !isSending && queue_front(tx_q) == NULL;
}

void jd_tx_init(void) {
    tx_q = queue_alloc(TXQ_SIZE);
}

void jd_send_low(jd_frame_t *f) {
    jd_compute_crc(f);
    queue_push(tx_q, f);
    jd_packet_ready();
}

int jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size) {
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
        bridge_forward_frame(f); // also forward packets we generate ourselves
    }

    return 0;
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