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
            pkt->service_index = 0;
            return true;
        }
        if (pkt->device_identifier == ctx->service_class_bl) {
            pkt->service_index = 1;
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
    case JD_GET(JD_CONTROL_REG_BOOTLOADER_PRODUCT_IDENTIFIER):
    case JD_GET(JD_CONTROL_REG_PRODUCT_IDENTIFIER):
        ctx->id_queued = pkt->service_command;
        break;
    }
}

static void process_frame(ctx_t *ctx, jd_frame_t *frame) {
    jd_packet_t *pkt = (jd_packet_t *)frame;
    if (pkt->service_index == 0)
        ctrl_handle_packet(ctx, pkt);
    else if (pkt->service_index == 1)
        bl_handle_packet(ctx, pkt);
}

void jd_prep_send(ctx_t *ctx) {
    jd_frame_t *frame = &ctx->txBuffer;
    frame->size = ((jd_packet_t *)frame)->service_size + 4;
    jd_compute_crc(frame);
    ctx->tx_full = 1;
}

int jd_process(ctx_t *ctx) {
    int rx_status = uart_rx(ctx, &ctx->rxBuffer, sizeof(ctx->rxBuffer));

    if (rx_status == RX_RECEPTION_OK) {
        if (valid_frame(ctx, &ctx->rxBuffer)) {
            LOG0_PULSE();
            ctx->rx_full = 1;
        }
        LOG0_PULSE();
        uart_post_rx(ctx);
        ctx->tx_start_time = 0;
        return 1;
    }

    if (rx_status == RX_LINE_BUSY) {
        LOG0_PULSE();
        ctx->tx_start_time = 0;
        return 1;
    }

    if (ctx->tx_full == 1 && !ctx->tx_start_time) {
        ctx->tx_start_time = ctx->now + 40 + (random(ctx) & 127);
    } else if (ctx->tx_start_time && ctx->tx_start_time <= ctx->now) {
        ctx->tx_start_time = 0;
        if (uart_tx(ctx, &ctx->txBuffer, JD_FRAME_SIZE(&ctx->txBuffer)) == 0) {
            // sent OK
            ctx->tx_full = 0;
            return 1;
        } else {
            // not sent because line was low
            // next loop iteration will pick it up as RX
            return 1;
        }
    }

    // only process frame when uart isn't busy
    if (!ctx->tx_full && ctx->rx_full) {
        process_frame(ctx, &ctx->rxBuffer);
        ctx->rx_full = 0;
        return 1;
    } else {
        identify(ctx);
        bl_process(ctx);
        return 0;
    }
}
