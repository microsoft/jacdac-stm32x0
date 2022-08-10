#include "jdstm.h"

#if defined(DUPLEX_UART)

#define RX_BUFFER_SIZE 512

#if defined(STM32G0) || defined(STM32WL)
#define NEW_UART 1
#else
#error "F0 not (yet?) supported for duplex UART"
#endif

#if DUPLEX_UART == 1
#define USARTx USART1
#define LL_DMAMUX_REQ_USARTx_RX LL_DMAMUX_REQ_USART1_RX
#define LL_DMAMUX_REQ_USARTx_TX LL_DMAMUX_REQ_USART1_TX
#elif DUPLEX_UART == 2
#define USARTx USART2
#define LL_DMAMUX_REQ_USARTx_RX LL_DMAMUX_REQ_USART2_RX
#define LL_DMAMUX_REQ_USARTx_TX LL_DMAMUX_REQ_USART2_TX
#else
#error "bad usart"
#endif

#if defined(STM32WL)
#define TX_AF LL_GPIO_AF_7
#define RX_AF LL_GPIO_AF_7
#elif defined(STM32G0)
#if PIN_RX == PB_7
#define RX_AF LL_GPIO_AF_0
#else
#define RX_AF LL_GPIO_AF_1
#endif
#if PIN_TX == PB_8
#define TX_AF LL_GPIO_AF_0
#else
#define TX_AF LL_GPIO_AF_1
#endif
#else
#error "define AF"
#endif

#define DMA_CH_TX LL_DMA_CHANNEL_2
#define DMA_CH_RX LL_DMA_CHANNEL_3

#ifdef STM32G0
#define DMA_IRQn DMA1_Channel2_3_IRQn
#define DUART_DMA_Handler DMA1_Channel2_3_IRQHandler
#define NEW_UART 1
#elif defined(STM32WL)
#define DMA_IRQn DMA1_Channel2_IRQn
#define DMA_IRQn_2 DMA1_Channel3_IRQn
#define NEW_UART 1
// see DMA1_Channel2_IRQHandler below
#else
#error "define DMA"
#endif

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint16_t last_rx_ptr;
static cb_t tx_cb;
static data_cb_t rx_cb;

static void flush_rx(void) {
    for (;;) {
        unsigned startp = 0, endp = 0;
        target_disable_irq();
        uint32_t curr = RX_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, DMA_CH_RX);
        JD_ASSERT(curr != RX_BUFFER_SIZE); // should never happen in circular mode
        if (curr != last_rx_ptr) {
            //DMESG("c=%d",curr);
            startp = last_rx_ptr;
            if (curr < startp) {
                endp = RX_BUFFER_SIZE;
                last_rx_ptr = 0;
            } else {
                endp = curr;
                last_rx_ptr = curr;
            }
        }
        target_enable_irq();
        int sz = endp - startp;
        if (sz == 0)
            break;
        // DMESG("rx %d",sz);
        rx_cb(rx_buffer + startp, sz);
    }
}

void duart_process() {
    flush_rx();
}

void DUART_DMA_Handler(void) {
    flush_rx();
    dma_clear_flag(DMA_CH_RX, DMA_FLAG_TC | DMA_FLAG_HT);

    if (dma_has_flag(DMA_CH_TX, DMA_FLAG_TC)) {
        dma_clear_flag(DMA_CH_TX, DMA_FLAG_TC);
        LL_DMA_DisableChannel(DMA1, DMA_CH_TX);
        cb_t f = tx_cb;
        tx_cb = NULL;
        if (f)
            f();
    }
}

#ifdef STM32WL
void DMA1_Channel2_IRQHandler(void) {
    DUART_DMA_Handler();
}
void DMA1_Channel3_IRQHandler(void) {
    DUART_DMA_Handler();
}
#endif

void duart_init(data_cb_t cb) {
    rx_cb = cb;

    DMA_CLK_ENABLE();

    NVIC_SetPriority(DMA_IRQn, IRQ_PRIORITY_DMA);
    NVIC_EnableIRQ(DMA_IRQn);
#ifdef DMA_IRQn_2
    NVIC_SetPriority(DMA_IRQn_2, IRQ_PRIORITY_DMA);
    NVIC_EnableIRQ(DMA_IRQn_2);
#endif

#if DUPLEX_UART == 2
    __HAL_RCC_USART2_CLK_ENABLE();
#ifdef STM32WL
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_HSI);
#elif !defined(DISABLE_PLL)
#error "PLL not supported"
#endif
#elif DUPLEX_UART == 1
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_HSI);
    __HAL_RCC_USART1_CLK_ENABLE();
#else
#error "bad usart"
#endif

    pin_setup_output_af(PIN_TX, TX_AF);
    pin_setup_output_af(PIN_RX, RX_AF);
    pin_set_pull(PIN_RX, PIN_PULL_UP);

    /* USART_RX Init */
#if NEW_UART
    LL_DMA_SetPeriphRequest(DMA1, DMA_CH_RX, LL_DMAMUX_REQ_USARTx_RX);
#endif
    LL_DMA_ConfigTransfer(DMA1, DMA_CH_RX,
                          LL_DMA_DIRECTION_PERIPH_TO_MEMORY | //
                              LL_DMA_PRIORITY_HIGH |          //
                              LL_DMA_MODE_CIRCULAR |          //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);
    /* USART_TX Init */
#if NEW_UART
    LL_DMA_SetPeriphRequest(DMA1, DMA_CH_TX, LL_DMAMUX_REQ_USARTx_TX);
#endif
    LL_DMA_ConfigTransfer(DMA1, DMA_CH_TX,
                          LL_DMA_DIRECTION_MEMORY_TO_PERIPH | //
                              LL_DMA_PRIORITY_HIGH |          //
                              LL_DMA_MODE_NORMAL |            //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_EnableIT_TC(DMA1, DMA_CH_TX);
    LL_DMA_EnableIT_TC(DMA1, DMA_CH_RX);
    LL_DMA_EnableIT_HT(DMA1, DMA_CH_RX);

#if NEW_UART
    USARTx->CR1 = LL_USART_DATAWIDTH_8B | LL_USART_PARITY_NONE | LL_USART_OVERSAMPLING_16 |
                  LL_USART_DIRECTION_TX_RX;
    USARTx->BRR = HSI_MHZ; // ->1MHz (16x oversampling)
#else
    USARTx->CR1 = LL_USART_DATAWIDTH_8B | LL_USART_PARITY_NONE | LL_USART_OVERSAMPLING_8 |
                  LL_USART_DIRECTION_TX_RX;
    USARTx->BRR = HSI_MHZ * 2; // ->1MHz (8x oversampling)
#endif

    USARTx->CR2 = LL_USART_STOPBITS_1;
    USARTx->CR3 = LL_USART_HWCONTROL_NONE;

#ifdef LL_USART_PRESCALER_DIV1
    LL_USART_SetPrescaler(USARTx, LL_USART_PRESCALER_DIV1);
#endif

    LL_DMA_ConfigAddresses(DMA1, DMA_CH_RX, (uint32_t) & (USARTx->RDR), (uint32_t)rx_buffer,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, DMA_CH_RX, RX_BUFFER_SIZE);
    LL_USART_EnableDMAReq_RX(USARTx);
    LL_DMA_EnableChannel(DMA1, DMA_CH_RX);

    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_TEACK(USARTx)))
        ;

    pwr_enter_no_sleep();
}

void duart_start_tx(const void *data, uint32_t numbytes, cb_t done_handler) {
    JD_ASSERT(tx_cb == NULL);
    tx_cb = done_handler;

    LL_DMA_ConfigAddresses(DMA1, DMA_CH_TX, (uint32_t)data, (uint32_t) & (USARTx->TDR),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, DMA_CH_TX, numbytes);
    LL_USART_EnableDMAReq_TX(USARTx);
    LL_DMA_EnableChannel(DMA1, DMA_CH_TX);
}

#endif
