#include "jdstm.h"

#if JD_USB_BRIDGE

static bool in_tx;

static void fill_buffer(void);
static void usbserial_done(void) {
    in_tx = 0;
    fill_buffer();
}

static void fill_buffer(void) {
    static uint8_t buf[64];

    target_disable_irq();
    int len = 0;
    if (!in_tx) {
        len = jd_usb_pull(buf);
        if (len > 0)
            in_tx = 1;
    }
    target_enable_irq();

    if (len > 0)
        duart_start_tx(buf, len, usbserial_done);
}

void jd_usb_pull_ready(void) {
    fill_buffer();
}

void usbserial_init() {
    duart_init(jd_usb_push);
}

void jd_usb_process(void) {
    duart_process();
}

#endif