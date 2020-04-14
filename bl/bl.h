#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "dmesg.h"
#include "pinnames.h"
#include "services.h"

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

    uint8_t *uart_data;
    uint32_t uart_bytesleft;
    uint8_t uart_mode;

    // timestamps
    uint32_t now;
    uint32_t low_start;
    uint32_t last_low;
    uint32_t led_off_time;

    jd_frame_t rxBuffer;
    jd_frame_t txBuffer;
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
