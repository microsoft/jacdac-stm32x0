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

static void rx_setup(void) {
    uartOwnsPin();
    LL_USART_DisableDirectionTx(USARTx);
    LL_USART_EnableDirectionRx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_REACK(USARTx)))
        ;
}

void uart_init(ctx_t *ctx) {
#if QUICK_LOG == 1
    ctx->log_reg = (volatile uint32_t *)&PORT(PIN_X0)->BSRR;
    ctx->log_p0 = PIN(PIN_X0);
    ctx->log_p1 = PIN(PIN_X1);
#endif

    LL_GPIO_SetPinPull(PIN_PORT, PIN_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinSpeed(PIN_PORT, PIN_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    uartDoesntOwnPin();

    USARTx->CR1 = LL_USART_DATAWIDTH_8B | LL_USART_PARITY_NONE | LL_USART_OVERSAMPLING_8 |
                  LL_USART_DIRECTION_TX;
    USARTx->BRR = CPU_MHZ * 2; // ->1MHz

#ifdef LL_USART_PRESCALER_DIV1
    LL_USART_SetPrescaler(USARTx, LL_USART_PRESCALER_DIV1);
#endif

    LL_USART_ConfigHalfDuplexMode(USARTx);

    rx_setup();
}

void uart_disable(ctx_t *ctx) {
    LL_USART_Disable(USARTx);
    uartDoesntOwnPin();
}

int uart_rx(ctx_t *ctx, void *data, uint32_t maxbytes) {
    uint32_t isr = USARTx->ISR;

    if (!ctx->low_detected && !(isr & USART_ISR_FE)) {
        if (isr & USART_ISR_BUSY)
            return RX_LINE_BUSY;
        return RX_LINE_IDLE;
    }

    ctx->low_detected = 0;

    // wait for BUSY to be cleared - ie the low pulse has ended
    while (isr & USART_ISR_BUSY)
        isr = USARTx->ISR;

    uart_post_rx(ctx); // clear errors

    uint8_t *dp = data;
    int32_t delaycnt = -10 * CPU_MHZ; // allow a bit more delay at the start
    LOG0_PULSE();
    while (maxbytes > 0) {
        isr = USARTx->ISR;
        if (isr & USART_ISR_RXNE) {
            *dp++ = USARTx->RDR;
            maxbytes--;
            delaycnt = 0;
        } else if (isr & USART_ISR_FE || delaycnt > 4 * CPU_MHZ) {
            break;
        } else {
            delaycnt++;
        }
    }
    LOG0_PULSE();

    return RX_RECEPTION_OK;
}

void uart_post_rx(ctx_t *ctx) {
    (void)USARTx->RDR;
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE;
    (void)USARTx->RDR;
}

int uart_tx(ctx_t *ctx, const void *data, uint32_t numbytes) {
    LL_USART_Disable(USARTx);
    uartDoesntOwnPin();

    LL_GPIO_ResetOutputPin(PIN_PORT, PIN_PIN);
    gpio_probe_and_set(PIN_PORT, PIN_PIN, PIN_MODER | PIN_PORT->MODER);
    if (!(PIN_PORT->MODER & PIN_MODER)) {
        ctx->low_detected = 1;
        rx_setup();
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

    const uint8_t *sp = data;
    target_wait_us(40);

    while (numbytes > 0) {
        uint32_t isr = USARTx->ISR;
        if (isr & USART_ISR_TXE) {
            numbytes--;
            USARTx->TDR = *sp++;
        }
    }

    while (!LL_USART_IsActiveFlag_TC(USARTx))
        ;

    LL_USART_Disable(USARTx);
    LL_GPIO_SetPinMode(PIN_PORT, PIN_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_ResetOutputPin(PIN_PORT, PIN_PIN);
    target_wait_us(12);
    LL_GPIO_SetOutputPin(PIN_PORT, PIN_PIN);
    rx_setup();

    return 0;
}
