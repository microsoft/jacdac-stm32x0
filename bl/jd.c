#include "bl.h"

#define ST_IDLE 0
#define ST_IN_TX 1
#define ST_IN_RX 2

static void process_frame(ctx_t *ctx, jd_frame_t *frame) {}

void jd_process(ctx_t *ctx) {
    if (ctx->uart_mode == UART_MODE_NONE) {
        if (pin_get(UART_PIN)) {
            if (ctx->low_start && 9 <= ctx->now - ctx->low_start &&
                ctx->now - ctx->low_start <= 50) {
                uart_start_rx(ctx, &ctx->rxBuffer, sizeof(ctx->rxBuffer));
            }
            ctx->low_start = 0;
        } else {
            ctx->last_low = ctx->now;
            if (!ctx->low_start)
                ctx->low_start = ctx->now;
        }
    } else {
        switch (uart_process(ctx)) {
        case UART_END_RX:
            process_frame(ctx, &ctx->rxBuffer);
            break;
        case UART_END_TX:
            break;
        }
    }

    if (ctx->now == 1) {

    } else if (ctx->now == 2) {
        if (uart_start_tx(ctx, &ctx->txBuffer, sizeof(ctx->txBuffer)) == 0) {
            // sent OK
        }
    }
}
