#include "bl.h"

#ifndef HW_TYPE
#define HW_TYPE 0x0
#endif

static int setup_tx(ctx_t *ctx, int cmd, const void *data, int size) {
    if (ctx->tx_full)
        return -1;

    jd_packet_t *pkt = (jd_packet_t *)&ctx->txBuffer;
    pkt->service_command = BL_CMD_PAGE_DATA;
    pkt->service_size = size;
    memcpy(pkt->data, data, size);
    ctx->tx_full = 1;

    return 0;
}

static const uint32_t bl_ad_data[] = {
    BL_PAGE_SIZE,
    FLASH_SIZE,
    HW_TYPE,
};

void bl_process(ctx_t *ctx) {
    if (ctx->subpageno == 0xff && setup_tx(ctx, BL_CMD_PAGE_DATA, &ctx->subpageerr, 8) == 0)
        ctx->subpageno = 0;
    if (ctx->bl_ad_queued &&
        setup_tx(ctx, JD_CMD_ADVERTISEMENT_DATA, bl_ad_data, sizeof(bl_ad_data)) == 0)
        ctx->bl_ad_queued = 0;
}

static void page_data(ctx_t *ctx, struct bl_page_data *d, int datasize) {
    if (datasize <= 0) {
        ctx->subpageerr = 1;
        return;
    }
    if (d->subpageno == 0) {
        memset(ctx->pagedata, 0, BL_PAGE_SIZE);
        ctx->subpageerr = 0;
        ctx->subpageno = 1;
        ctx->pageaddr = d->pageaddr;
    } else if (!ctx->subpageerr && d->subpageno == ctx->subpageno && d->pageaddr == ctx->pageaddr) {
        ctx->subpageno++;
    } else {
        ctx->subpageerr = 1;
    }

    bool isFinal = d->subpageno == d->subpagemax;

    if (!ctx->subpageerr) {
        memcpy((uint8_t *)ctx->pageaddr + d->pageoffset, d->data, datasize);
        if (isFinal) {
            flash_erase((void *)ctx->pageaddr);
            flash_program((void *)ctx->pageaddr, ctx->pagedata, BL_PAGE_SIZE);
        }
    }

    if (isFinal)
        ctx->subpageno = 0xff; // send ack next round
}

void bl_handle_packet(ctx_t *ctx, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_CMD_ADVERTISEMENT_DATA:
        ctx->bl_ad_queued = 1;
        break;
    case BL_CMD_PAGE_DATA:
        page_data(ctx, (struct bl_page_data *)pkt->data, pkt->service_size - 28);
        break;
    }
}
