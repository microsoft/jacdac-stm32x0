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
    ctx->jd_status_reg = (volatile uint16_t *)&PORT(UART_PIN)->IDR;
    ctx->jd_status--;

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
}

void uart_disable(ctx_t *ctx) {
    LL_USART_Disable(USARTx);
    uartDoesntOwnPin();
}

void uart_rx(ctx_t *ctx, void *data, uint32_t maxbytes) {
    uartOwnsPin();
    LL_USART_DisableDirectionTx(USARTx);
    LL_USART_EnableDirectionRx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_REACK(USARTx)))
        ;

    uint8_t *dp = data;
    uint32_t delaycnt = 0;
    while (maxbytes > 0) {
        uint32_t isr = USARTx->ISR;
        if (isr & USART_ISR_RXNE) {
            *dp++ = USARTx->RDR;
            maxbytes--;
            delaycnt = 0;
        } else if (isr & USART_ISR_FE || delaycnt > 100000) {
            break;
        } else {
            delaycnt++;
        }
    }

    uart_disable(ctx);
}

int uart_tx(ctx_t *ctx, const void *data, uint32_t numbytes) {
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
    uart_disable(ctx);

    return 0;
}
