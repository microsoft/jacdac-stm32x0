#include "bl.h"

#define ST_IDLE 0
#define ST_IN_TX 1
#define ST_IN_RX 2

static bool valid_frame(ctx_t *ctx, jd_frame_t *frame) {
    if (jd_crc16((uint8_t *)frame + 2, JD_FRAME_SIZE(frame) - 2) != frame->crc)
        return false;

    jd_packet_t *pkt = (jd_packet_t *)frame;

    if (!(pkt->flags & JD_FRAME_FLAG_COMMAND) || (pkt->flags & JD_FRAME_FLAG_VNEXT))
        return false;

    if (pkt->device_identifier == BL_DEVICE_ID)
        return true;

    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        if (pkt->device_identifier == JD_SERVICE_CLASS_CONTROL) {
            pkt->service_number = 0;
            return true;
        }
        if (pkt->device_identifier == ctx->service_class_bl) {
            pkt->service_number = 1;
            return true;
        }
    }

    return false;
}

static void identify(ctx_t *ctx) {
    if (!ctx->id_counter)
        return;
    if (ctx->next_id_blink <= ctx->now) {
        ctx->next_id_blink = ctx->now + 150000;
        ctx->id_counter--;
        led_blink(50000);
    }
}

static void ctrl_handle_packet(ctx_t *ctx, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_CMD_ANNOUNCE:
        ctx->next_announce = ctx->now;
        break;
    case JD_CONTROL_CMD_IDENTIFY:
        ctx->id_counter = 7;
        ctx->next_id_blink = ctx->now;
        identify(ctx);
        break;
    case JD_CONTROL_CMD_RESET:
        target_reset();
        break;
    }
}

static void process_frame(ctx_t *ctx, jd_frame_t *frame) {
    jd_packet_t *pkt = (jd_packet_t *)frame;
    if (pkt->service_number == 0)
        ctrl_handle_packet(ctx, pkt);
    else if (pkt->service_number == 1)
        bl_handle_packet(ctx, pkt);
}

void jd_prep_send(ctx_t *ctx) {
    jd_frame_t *frame = &ctx->txBuffer;
    frame->size = ((jd_packet_t *)frame)->service_size + 4;
    jd_compute_crc(frame);
    ctx->tx_full = 1;
}

void jd_process(ctx_t *ctx) {
    uint32_t now = ctx->now;

    if (pin_get(UART_PIN)) {
        if (!ctx->rx_full && ctx->low_start && 9 <= now - ctx->low_start &&
            now - ctx->low_start <= 50) {
            ctx->rx_full = 1;
            uart_rx(ctx, &ctx->rxBuffer, sizeof(ctx->rxBuffer));
            if (!valid_frame(ctx, &ctx->rxBuffer))
                ctx->rx_full = 0;
        } else if (ctx->tx_full == 1 && !ctx->tx_start_time) {
            ctx->tx_start_time = now + 40 + (random(ctx) & 127);
        } else if (ctx->tx_start_time && ctx->tx_start_time <= now) {
            ctx->tx_start_time = 0;
            if (uart_tx(ctx, &ctx->txBuffer, JD_FRAME_SIZE(&ctx->txBuffer)) == 0) {
                // sent OK
                ctx->tx_full = 0;
            } else {
                // not sent because line was low
                // next loop iteration will pick it up as RX
            }
        }
        ctx->low_start = 0;
    } else {
        if (!ctx->low_start)
            ctx->low_start = now;
        ctx->tx_start_time = 0;
    }

    now = ctx->now = tim_get_micros();

    // only process frame when uart isn't busy
    if (!ctx->tx_full && ctx->rx_full) {
        process_frame(ctx, &ctx->rxBuffer);
        ctx->rx_full = 0;
    }

    identify(ctx);
    bl_process(ctx);
}
