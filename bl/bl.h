#pragma once

#include "blutil.h"

typedef void (*cb_t)(void);

typedef struct ctx {
    uint8_t jd_state;

    uint8_t tx_full, rx_full;
    uint8_t chunk_no;
    uint8_t bl_ad_queued;
    uint8_t id_counter;
    uint8_t low_detected;
    uint16_t id_queued;

#if QUICK_LOG == 1
    volatile uint32_t *log_reg;
    uint32_t log_p0;
    uint32_t log_p1;
#endif

    // these three fields are sent directly from here, so don't move them
    uint32_t session_id;
    uint32_t subpageerr;
    uint32_t page_address;

    uint32_t randomseed;
    uint32_t service_class_bl;

    // timestamps
    uint32_t now;
    uint32_t tx_start_time;
    uint32_t led_on_time;
    uint32_t next_announce;
    uint32_t next_id_blink;
    uint32_t app_start_time;
    uint32_t rx_timeout;

    jd_frame_t rxBuffer;
    jd_frame_t txBuffer;

    uint8_t pagedata[JD_FLASH_PAGE_SIZE];
} ctx_t;

#if QUICK_LOG == 1
#define LOG0_ON() *ctx->log_reg = ctx->log_p0
#define LOG1_ON() *ctx->log_reg = ctx->log_p1
#define LOG0_OFF() *ctx->log_reg = ctx->log_p0 << 16
#define LOG1_OFF() *ctx->log_reg = ctx->log_p1 << 16
#else
#define LOG0_ON() ((void)0)
#define LOG1_ON() ((void)0)
#define LOG0_OFF() ((void)0)
#define LOG1_OFF() ((void)0)
#endif
#define LOG0_PULSE()                                                                               \
    do {                                                                                           \
        LOG0_ON();                                                                                 \
        LOG0_OFF();                                                                                \
    } while (0)
#define LOG1_PULSE()                                                                               \
    do {                                                                                           \
        LOG1_ON();                                                                                 \
        LOG1_OFF();                                                                                \
    } while (0)

extern ctx_t ctx_;

int jd_process(ctx_t *ctx);
void jd_prep_send(ctx_t *ctx);

void tim_init(void);
uint32_t tim_get_micros(void);

void uart_init(ctx_t *ctx);
int uart_tx(ctx_t *ctx, const void *data, uint32_t numbytes);
#define RX_LINE_BUSY 1
#define RX_LINE_IDLE 2
#define RX_RECEPTION_OK 3
int uart_rx(ctx_t *ctx, void *data, uint32_t maxbytes);
void uart_post_rx(ctx_t *ctx);

uint32_t random(ctx_t *);
uint16_t jd_crc16(const void *data, uint32_t size);
void jd_compute_crc(jd_frame_t *frame);

void bl_handle_packet(ctx_t *ctx, jd_packet_t *pkt);
void bl_process(ctx_t *ctx);
bool bl_fixup_app_handlers(ctx_t *ctx);
void led_blink(int us);

#define BL_MAGIC_FLAG_APP 0x873d9293
extern uint32_t _estack;
#define BL_MAGIC_FLAG (&_estack)[-1]

#define BL_DEVICE_ID ctx->txBuffer.device_identifier
