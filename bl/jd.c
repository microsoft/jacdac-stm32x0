#include "bl.h"

jd_frame_t rxBuffer;
jd_frame_t txBuffer;

void jd_process() {
    switch (uart_process()) {
    case 1: // rx end
        break;
    case 2: // tx end
        break;
    }

    if (now == 1) {
        uart_start_rx(&rxBuffer, sizeof(rxBuffer));
    } else if (now == 2) {
        if (uart_start_tx(&txBuffer, sizeof(txBuffer)) == 0) {
            // sent OK
        }
    }
}
