#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "dmesg.h"
#include "pinnames.h"
#include "services.h"

#include "blproto.h"

#if defined(STM32F0)
#include "stm32f0.h"
#elif defined(STM32G0)
#include "stm32g0.h"
#else
#error "invalid CPU"
#endif

#include "jdprotocol.h"
#include "blhw.h"

#define CPU_MHZ PLL_MHZ

typedef void (*cb_t)(void);

typedef struct ctx {
    uint8_t jd_state;

    uint8_t uart_mode;

    uint8_t tx_full, rx_full;
    uint8_t subpageno;
    uint8_t bl_ad_queued;
    uint8_t id_counter;

    // these two fields are sent directly from here, so don't move them
    uint32_t subpageerr;
    uint32_t pageaddr;

    uint32_t randomseed;
    uint32_t service_class_bl;

    uint8_t *uart_data;
    uint32_t uart_bytesleft;

    // timestamps
    uint32_t now;
    uint32_t low_start;
    uint32_t tx_start_time;
    uint32_t led_off_time;
    uint32_t next_announce;
    uint32_t next_id_blink;
    uint32_t app_start_time;

    jd_frame_t rxBuffer;
    jd_frame_t txBuffer;

    uint8_t pagedata[FLASH_PAGE_SIZE];
} ctx_t;

extern ctx_t ctx_;

void jd_process(ctx_t *ctx);

void tim_init(void);
uint32_t tim_get_micros(void);

#define UART_MODE_NONE 0
#define UART_MODE_RX 1
#define UART_MODE_TX 2

void uart_init(ctx_t *ctx);
int uart_start_tx(ctx_t *ctx, const void *data, uint32_t numbytes);
void uart_start_rx(ctx_t *ctx, void *data, uint32_t maxbytes);
#define UART_END_TX 1
#define UART_END_RX 2
int uart_process(ctx_t *ctx);

uint16_t crc16(const void *data, uint32_t size);

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

#ifndef HW_TYPE
#define HW_TYPE 0
#endif

#define BL_DEVICE_ID ctx->txBuffer.device_identifier
