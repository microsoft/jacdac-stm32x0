#include "bl.h"

#ifdef USART_ISR_TXE_TXFNF
#define USART_ISR_TXE USART_ISR_TXE_TXFNF
#define USART_ISR_RXNE USART_ISR_RXNE_RXFNE
#endif

#define PORT(pin) ((GPIO_TypeDef *)(GPIOA_BASE + (0x400 * (pin >> 4))))
#define PIN(pin) (1 << ((pin)&0xf))

#define PIN_PORT PORT(UART_PIN)
#define PIN_PIN PIN(UART_PIN)
#define PIN_MODER (1 << (UART_PIN & 0xf) * 2)

#if USART_IDX == 1
#define USARTx USART1
#elif USART_IDX == 2
#define USARTx USART2
#else
#error "bad usart"
#endif

// This is extracted to a function to make sure the compiler doesn't
// insert stuff between checking the input pin and setting the mode.
// This results in a collision window of around 90ns
__attribute__((noinline)) void gpio_probe_and_set(GPIO_TypeDef *gpio, uint32_t pin, uint32_t mode) {
    if (gpio->IDR & pin)
        gpio->MODER = mode;
}

static void uartOwnsPin(void) {
    LL_GPIO_SetPinMode(PIN_PORT, PIN_PIN, LL_GPIO_MODE_ALTERNATE);
    if ((UART_PIN & 0xf) <= 7)
        LL_GPIO_SetAFPin_0_7(PIN_PORT, PIN_PIN, UART_PIN_AF);
    else
        LL_GPIO_SetAFPin_8_15(PIN_PORT, PIN_PIN, UART_PIN_AF);
}

static void uartDoesntOwnPin(void) {
    LL_GPIO_SetPinMode(PIN_PORT, PIN_PIN, LL_GPIO_MODE_INPUT);
}

void uart_init(ctx_t *ctx) {
    LL_GPIO_SetPinPull(PIN_PORT, PIN_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinSpeed(PIN_PORT, PIN_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    uartDoesntOwnPin();

    USARTx->CR1 = LL_USART_DATAWIDTH_8B | LL_USART_PARITY_NONE | LL_USART_OVERSAMPLING_16 |
                  LL_USART_DIRECTION_TX;
    USARTx->BRR = CPU_MHZ; // ->1MHz

#ifdef LL_USART_PRESCALER_DIV1
    LL_USART_SetPrescaler(USARTx, LL_USART_PRESCALER_DIV1);
#endif

#ifdef LL_USART_FIFOTHRESHOLD_1_8
    //LL_USART_SetTXFIFOThreshold(USARTx, LL_USART_FIFOTHRESHOLD_1_8);
    //LL_USART_SetRXFIFOThreshold(USARTx, LL_USART_FIFOTHRESHOLD_1_8);
    //LL_USART_DisableFIFO(USARTx);
#endif

    LL_USART_ConfigHalfDuplexMode(USARTx);
}

void uart_disable(ctx_t *ctx) {
    LL_USART_Disable(USARTx);
    uartDoesntOwnPin();
    ctx->uart_mode = UART_MODE_NONE;
}

void uart_start_rx(ctx_t *ctx, void *data, uint32_t maxbytes) {
    uartOwnsPin();
    LL_USART_DisableDirectionTx(USARTx);
    LL_USART_EnableDirectionRx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_REACK(USARTx)))
        ;

    ctx->uart_data = data;
    ctx->uart_bytesleft = maxbytes;
    ctx->rx_timeout = ctx->now + maxbytes * 15;
    ctx->uart_mode = UART_MODE_RX;
}

int uart_process(ctx_t *ctx) {
    uint32_t isr = USARTx->ISR;
    if (ctx->uart_mode == UART_MODE_RX) {
        if (isr & USART_ISR_FE || ctx->now > ctx->rx_timeout) {
            uart_disable(ctx);
            return UART_END_RX;
        } else if (isr & USART_ISR_RXNE) {
            uint8_t c = USARTx->RDR;
            if (ctx->uart_bytesleft) {
                ctx->uart_bytesleft--;
                *ctx->uart_data++ = c;
            }
        }
    } else if (ctx->uart_mode == UART_MODE_TX) {
        if (isr & USART_ISR_TXE) {
            if (ctx->uart_bytesleft) {
                ctx->uart_bytesleft--;
                USARTx->TDR = *ctx->uart_data++;
            } else {
                if (!LL_USART_IsActiveFlag_TC(USARTx))
                    return 0;
                LL_USART_Disable(USARTx);
                LL_GPIO_SetPinMode(PIN_PORT, PIN_PIN, LL_GPIO_MODE_OUTPUT);
                LL_GPIO_ResetOutputPin(PIN_PORT, PIN_PIN);
                target_wait_us(12);
                LL_GPIO_SetOutputPin(PIN_PORT, PIN_PIN);
                uart_disable(ctx);
                return UART_END_TX;
            }
        }
    }
    return 0;
}

int uart_start_tx(ctx_t *ctx, const void *data, uint32_t numbytes) {
    LL_GPIO_ResetOutputPin(PIN_PORT, PIN_PIN);
    gpio_probe_and_set(PIN_PORT, PIN_PIN, PIN_MODER | PIN_PORT->MODER);
    if (!(PIN_PORT->MODER & PIN_MODER)) {
        return -1;
    }
    target_wait_us(11);
    LL_GPIO_SetOutputPin(PIN_PORT, PIN_PIN);

    uartOwnsPin();
    LL_USART_DisableDirectionRx(USARTx);
    LL_USART_EnableDirectionTx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_TEACK(USARTx)))
        ;

    ctx->uart_data = (void *)data;
    ctx->uart_bytesleft = numbytes;
    ctx->uart_mode = UART_MODE_TX;

    target_wait_us(37);

    return 0;
}
