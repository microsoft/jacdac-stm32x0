#include "jdstm.h"

// Implemented based on ideas from https://cnnblike.com/post/stm32-OneWire/
// Basically, uses UART at 125kHz for signalling and at 10kHz for reset
// UART is configured as "half-duplex", meaning the TX/RX pins are connected internally,
// but in fact we sometimes enable both transmitter and receiver.
// The TX pin is configured with pull-up as open-drain.
// Unlike in the article above, no external diode is necessary, and the RX pin is not used.

#ifdef ONE_WIRE

#if USART_IDX == 1
#define OW_UART 2
#else
#define OW_UART 1
#endif

#if OW_UART == 1
#define USARTx USART1
#elif OW_UART == 2
#define USARTx USART2
#else
#error "bad usart"
#endif

void one_init(void) {
#if OW_UART == 2
#ifndef DISABLE_PLL
#error "PLL not supported"
#endif
    __HAL_RCC_USART2_CLK_ENABLE();
#elif OW_UART == 1
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_HSI);
    __HAL_RCC_USART1_CLK_ENABLE();
#else
#error "bad usart"
#endif

    pin_setup_output_af(PIN_TX, PIN_TX_AF);
    pin_set_pull(PIN_TX, 1);
    pin_set_opendrain(PIN_TX);

    USARTx->CR1 = LL_USART_DATAWIDTH_8B | LL_USART_PARITY_NONE | LL_USART_OVERSAMPLING_16 |
                  LL_USART_DIRECTION_TX;

    USARTx->CR2 = LL_USART_STOPBITS_1;
    USARTx->CR3 = LL_USART_HWCONTROL_NONE;

#ifdef LL_USART_PRESCALER_DIV1
    LL_USART_SetPrescaler(USARTx, LL_USART_PRESCALER_DIV1);
#endif

    LL_USART_ConfigHalfDuplexMode(USARTx);
}

static void setup_xfer(int rx) {
    USARTx->BRR = rx == 2 ? HSI_MHZ * 100 : HSI_MHZ * 8; // ->10kHz or 125kHz
    if (rx)
        LL_USART_EnableDirectionRx(USARTx);
    else
        LL_USART_DisableDirectionRx(USARTx);
    LL_USART_EnableDirectionTx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_TEACK(USARTx)))
        ;
}

void one_write(uint8_t b) {
    setup_xfer(0);
    for (int i = 0; i < 8; ++i) {
        while (!LL_USART_IsActiveFlag_TXE(USARTx))
            ;
        LL_USART_TransmitData8(USARTx, (b & (1 << i)) ? 0xff : 0x00);
    }
    while (!LL_USART_IsActiveFlag_TC(USARTx))
        ;
    LL_USART_Disable(USARTx);
}

uint8_t one_read(void) {
    setup_xfer(1);

    uint8_t r = 0;
    for (int i = 0; i < 8; ++i) {
        while (!LL_USART_IsActiveFlag_TXE(USARTx))
            ;
        LL_USART_TransmitData8(USARTx, 0xff);
        while (!LL_USART_IsActiveFlag_RXNE(USARTx))
            ;
        uint8_t v = LL_USART_ReceiveData8(USARTx);
        if (v == 0xff)
            r |= (1 << i);
    }
    LL_USART_Disable(USARTx);
    return r;
}

int one_reset(void) {
    int r;
    setup_xfer(2);
    LL_USART_TransmitData8(USARTx, 0xf0);
    while (!LL_USART_IsActiveFlag_RXNE(USARTx))
        ;
    uint8_t v = LL_USART_ReceiveData8(USARTx);
    r = v == 0xf0 ? -1 : 0;
    LL_USART_Disable(USARTx);
    return r;
}

#endif // ONE_WIRE
