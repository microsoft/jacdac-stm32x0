#include "bl.h"

static int setup_tx(ctx_t *ctx, int cmd, const void *data, int size) {
    if (ctx->tx_full)
        return -1;

    jd_packet_t *pkt = (jd_packet_t *)&ctx->txBuffer;
    pkt->service_command = cmd;
    pkt->service_size = size;
    pkt->service_index = 1;
    memcpy(pkt->data, data, size);
    jd_prep_send(ctx);

    return 0;
}

static const uint32_t bl_ad_data[] = {JD_SERVICE_CLASS_BOOTLOADER, BL_PAGE_SIZE,
                                      JD_FLASH_SIZE - BL_SIZE, 0};

void bl_process(ctx_t *ctx) {
    if (ctx->chunk_no == 0xff && setup_tx(ctx, JD_BOOTLOADER_CMD_PAGE_DATA, &ctx->session_id, 12) == 0)
        ctx->chunk_no = 0;
    if (ctx->bl_ad_queued &&
        setup_tx(ctx, JD_CMD_ANNOUNCE, bl_ad_data, sizeof(bl_ad_data)) == 0) {
        // append our device class
        ((uint32_t *)ctx->txBuffer.data)[4] = bl_dev_info.device_class;
        jd_prep_send(ctx);
        ctx->bl_ad_queued = 0;
    }
}

static void bl_write_page(ctx_t *ctx) {
    if (ctx->page_address == 0x8000000) {
        struct app_top_handlers *a = (struct app_top_handlers *)ctx->pagedata;
        memcpy(&a->devinfo, &bl_dev_info, sizeof(bl_dev_info));
        a->boot_reset_handler = (uint32_t)&bl_dev_info + 32 + 1; // +1 for thumb state
    }
    flash_erase((void *)ctx->page_address);
    flash_program((void *)ctx->page_address, ctx->pagedata, BL_PAGE_SIZE);
}

bool bl_fixup_app_handlers(ctx_t *ctx) {
    bool can_start = (app_handlers->app_reset_handler >> 24) == 0x08;
    if (can_start && app_dev_info.magic + 1 == 0) {
        ctx->page_address = 0x8000000;
        memcpy(ctx->pagedata, (void *)ctx->page_address, BL_PAGE_SIZE);
        bl_write_page(ctx);
        return true;
    } else {
        return can_start;
    }
}

static void page_data(ctx_t *ctx, jd_bootloader_page_data_t *d, int datasize) {
    if (ctx->session_id != d->session_id)
        return;

    if (datasize < 0) {
        ctx->subpageerr = 1;
        return;
    }

    if (d->chunk_no == 0) {
        memset(ctx->pagedata, 0, BL_PAGE_SIZE);
        ctx->subpageerr = 0;
        ctx->chunk_no = 1;
        ctx->page_address = d->page_address;
        if (d->page_address & (BL_PAGE_SIZE - 1))
            ctx->subpageerr = 4;
        uint32_t off = d->page_address - 0x8000000;
        if (off > (uint32_t)&bl_dev_info - BL_PAGE_SIZE)
            ctx->subpageerr = 2;
    } else if (!ctx->subpageerr && d->chunk_no == ctx->chunk_no && d->page_address == ctx->page_address) {
        ctx->chunk_no++;
    } else {
        if (ctx->subpageerr == 0)
            ctx->subpageerr = (ctx->chunk_no << 16) + d->chunk_no;
    }

    if (d->page_offset >= BL_PAGE_SIZE || d->page_offset + datasize > BL_PAGE_SIZE)
        ctx->subpageerr = 3;

    bool isFinal = d->chunk_no == d->chunk_max;

    if (!ctx->subpageerr) {
        memcpy(ctx->pagedata + d->page_offset, d->page_data, datasize);
        if (isFinal)
            bl_write_page(ctx);
    }

    if (isFinal)
        ctx->chunk_no = 0xff; // send ack next round
}

void bl_handle_packet(ctx_t *ctx, jd_packet_t *pkt) {
    ctx->app_start_time = 0x80000000; // prevent app start
    switch (pkt->service_command) {
    case JD_BOOTLOADER_CMD_INFO:
        ctx->bl_ad_queued = 1;
        break;
    case JD_BOOTLOADER_CMD_PAGE_DATA:
        page_data(ctx, (jd_bootloader_page_data_t *)pkt->data, pkt->service_size - 28);
        break;
    case JD_BOOTLOADER_CMD_SET_SESSION:
        ctx->session_id = *(uint32_t *)pkt->data;
        ctx->chunk_no = 0xff;
        break;
    }
}
