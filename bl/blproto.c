#include "bl.h"

static int setup_tx(ctx_t *ctx, int cmd, const void *data, int size) {
    if (ctx->tx_full)
        return -1;

    jd_packet_t *pkt = (jd_packet_t *)&ctx->txBuffer;
    pkt->service_command = cmd;
    pkt->service_size = size;
    pkt->service_number = 1;
    memcpy(pkt->data, data, size);
    ctx->tx_full = 1;

    return 0;
}

static const uint32_t bl_ad_data[] = {
    JD_SERVICE_CLASS_BOOTLOADER,
    BL_PAGE_SIZE,
    FLASH_SIZE - BL_SIZE,
    0,
};

void bl_process(ctx_t *ctx) {
    if (ctx->subpageno == 0xff && setup_tx(ctx, BL_CMD_PAGE_DATA, &ctx->subpageerr, 8) == 0)
        ctx->subpageno = 0;
    if (ctx->bl_ad_queued &&
        setup_tx(ctx, JD_CMD_ADVERTISEMENT_DATA, bl_ad_data, sizeof(bl_ad_data)) == 0) {
        ((uint32_t *)ctx->txBuffer.data)[3] = bl_dev_info.device_class;
        ctx->bl_ad_queued = 0;
    }
}

static void bl_write_page(ctx_t *ctx) {
    if (ctx->pageaddr == 0x8000000) {
        struct app_top_handlers *a = (struct app_top_handlers *)ctx->pagedata;
        memcpy(&a->devinfo, &bl_dev_info, sizeof(bl_dev_info));
        if (a->app_reset_handler == 0 || a->app_reset_handler + 1 == 0)
            a->app_reset_handler = a->boot_reset_handler;
        a->boot_reset_handler =
            (uint32_t)&bl_dev_info + 32 + 1; // +1 for thumb state
    }
    flash_erase((void *)ctx->pageaddr);
    flash_program((void *)ctx->pageaddr, ctx->pagedata, BL_PAGE_SIZE);
}

bool bl_fixup_app_handlers(ctx_t *ctx) {
    if (app_dev_info.magic + 1 == 0 && (app_handlers->app_reset_handler >> 24) == 0x08) {
        ctx->pageaddr = 0x8000000;
        memcpy(ctx->pagedata, (void *)ctx->pageaddr, BL_PAGE_SIZE);
        bl_write_page(ctx);
        return true;
    } else {
        return false;
    }
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
        if (d->pageaddr & (BL_PAGE_SIZE - 1))
            ctx->subpageerr = 4;
        uint32_t off = d->pageaddr - 0x8000000;
        if (off > (uint32_t)&bl_dev_info - BL_PAGE_SIZE)
            ctx->subpageerr = 2;
    } else if (!ctx->subpageerr && d->subpageno == ctx->subpageno && d->pageaddr == ctx->pageaddr) {
        ctx->subpageno++;
    } else {
        ctx->subpageerr = 1000 * ctx->subpageno + d->subpageno;
    }

    if (d->pageoffset >= BL_PAGE_SIZE || d->pageoffset + datasize > BL_PAGE_SIZE)
        ctx->subpageerr = 3;

    bool isFinal = d->subpageno == d->subpagemax;

    if (!ctx->subpageerr) {
        memcpy(ctx->pagedata + d->pageoffset, d->data, datasize);
        if (isFinal)
            bl_write_page(ctx);
    }

    if (isFinal)
        ctx->subpageno = 0xff; // send ack next round
}

void bl_handle_packet(ctx_t *ctx, jd_packet_t *pkt) {
    ctx->app_start_time = 0x80000000; // prevent app start
    switch (pkt->service_command) {
    case JD_CMD_ADVERTISEMENT_DATA:
        ctx->bl_ad_queued = 1;
        break;
    case BL_CMD_PAGE_DATA:
        page_data(ctx, (struct bl_page_data *)pkt->data, pkt->service_size - 28);
        break;
    }
}
