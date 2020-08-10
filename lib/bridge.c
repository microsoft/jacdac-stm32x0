#include "lib.h"

void bridge_forward_frame(jd_frame_t *frame) {
    dspi_xfer(frame, NULL, JD_FRAME_SIZE(frame), NULL);
}

void bridge_init(uint8_t pin_cs, uint8_t pin_flow) {
    dspi_init();
}