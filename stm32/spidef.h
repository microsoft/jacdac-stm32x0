#pragma once

#if defined(STM32G031xx) || defined(STM32G030xx)
#if PIN_ASCK == PA_5 || PIN_ASCK == PA_1
#define SPI_IDX 1
#elif PIN_ASCK == PA_0
#define SPI_IDX 2
#else
    #error "Please add pin mapping for STM32G0 SPI"
#endif
#elif defined(STM32F031x6) || defined(STM32F030x4) || !defined(SPI2)
#define SPI_IDX 1
#else
#define SPI_IDX 2
#endif

#if SPI_IDX == 1
#define SPIx SPI1
#define IRQn SPI1_IRQn
#define IRQHandler SPI1_IRQHandler
#define LL_DMAMUX_REQ_SPIx_RX LL_DMAMUX_REQ_SPI1_RX
#define LL_DMAMUX_REQ_SPIx_TX LL_DMAMUX_REQ_SPI1_TX
#elif SPI_IDX == 2
#define SPIx SPI2
#define IRQn SPI2_IRQn
#define IRQHandler SPI2_IRQHandler
#define LL_DMAMUX_REQ_SPIx_RX LL_DMAMUX_REQ_SPI2_RX
#define LL_DMAMUX_REQ_SPIx_TX LL_DMAMUX_REQ_SPI2_TX
#else
#error "bad SPI"
#endif

#if SPI_IDX == 1
#define SPI_CLK_ENABLE __HAL_RCC_SPI1_CLK_ENABLE
#if PIN_ASCK == PA_5
#define PIN_AF LL_GPIO_AF_0
STATIC_ASSERT(PIN_ASCK == PA_5);

STATIC_ASSERT(PIN_AMOSI == PA_7 || PIN_AMOSI == PA_2);
#if SPI_RX
STATIC_ASSERT(PIN_AMISO == PA_6);
#endif
#elif PIN_ASCK == PA_1
#define PIN_AF LL_GPIO_AF_0
STATIC_ASSERT(PIN_ASCK == PA_1);
STATIC_ASSERT(PIN_AMOSI == PA_2);
#if SPI_RX
STATIC_ASSERT(PIN_AMISO == PA_6);
#endif
#else
#error "not supported"
#endif
#elif SPI_IDX == 2
#define SPI_CLK_ENABLE __HAL_RCC_SPI2_CLK_ENABLE
#if defined(STM32G031xx)
#if PIN_ASCK == PA_0
#define PIN_AF LL_GPIO_AF_0
#define PIN_AF_MOSI LL_GPIO_AF_1
STATIC_ASSERT(PIN_ASCK == PA_0);
STATIC_ASSERT(PIN_AMOSI == PB_7);
#if SPI_RX
// not implemented
STATIC_ASSERT(PIN_AMISO == -1);
#endif
#else
#error "not supported"
#endif
#else
#error "not supported"
#define SPI_CLK_ENABLE __HAL_RCC_SPI2_CLK_ENABLE
#ifdef STM32G0
#define PIN_SCK LL_GPIO_PIN_8
#define PIN_MOSI LL_GPIO_PIN_7
#define PIN_AF LL_GPIO_AF_1
#else
#define PIN_SCK LL_GPIO_PIN_13
#define PIN_MOSI LL_GPIO_PIN_15
#define PIN_AF LL_GPIO_AF_0
#endif
#endif
#else
#error "bad spi"
#endif

#ifdef STM32G0
#if SPI_RX
#define DMA_CH LL_DMA_CHANNEL_3
#define DMA_CH_RX LL_DMA_CHANNEL_2
#define DMA_IRQn DMA1_Channel2_3_IRQn
#define DMA_Handler DMA1_Channel2_3_IRQHandler
#else
#define DMA_CH LL_DMA_CHANNEL_1
#define DMA_IRQn DMA1_Channel1_IRQn
#define DMA_Handler DMA1_Channel1_IRQHandler
#endif
#else

#if SPI_IDX == 1
#define DMA_CH_RX LL_DMA_CHANNEL_2
#define DMA_CH LL_DMA_CHANNEL_3
#define DMA_IRQn DMA1_Channel2_3_IRQn
#define DMA_Handler DMA1_Channel2_3_IRQHandler
#elif SPI_IDX == 2
#define DMA_CH LL_DMA_CHANNEL_5
#define DMA_IRQn DMA1_Channel4_5_IRQn
#define DMA_Handler DMA1_Channel4_5_IRQHandler
#else
#error "bad spi"
#endif

#endif

#define DMA_FLAG_G DMA_ISR_GIF1
#define DMA_FLAG_TC DMA_ISR_TCIF1
#define DMA_FLAG_HT DMA_ISR_HTIF1
#define DMA_FLAG_TE DMA_ISR_TEIF1

#ifdef STM32G0
static inline void dma_clear_flag(int flag) {
    WRITE_REG(DMA1->IFCR, flag << ((DMA_CH) * 4));
}

static inline bool dma_has_flag(int flag) {
    return (READ_BIT(DMA1->ISR, flag << ((DMA_CH) * 4)) != 0);
}
#else
static inline void dma_clear_flag(int flag) {
    WRITE_REG(DMA1->IFCR, flag << ((DMA_CH - 1) * 4));
}

static inline bool dma_has_flag(int flag) {
    return (READ_BIT(DMA1->ISR, flag << ((DMA_CH - 1) * 4)) != 0);
}
#endif

static inline void spi_init0(void) {
    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    LL_SPI_Disable(SPIx);
    while(LL_SPI_IsEnabled(SPIx) == 1);

    pin_setup_output_af(PIN_ASCK, PIN_AF);

// some pins require an alternate AF enum for specific pins.
#ifdef PIN_AF_MOSI
    pin_setup_output_af(PIN_AMOSI, PIN_AF_MOSI);
#else
    pin_setup_output_af(PIN_AMOSI, PIN_AF);
#endif
}

static inline void spi_init1(void) {
    // LL_SPI_EnableIT_TXE(SPIx);
    // LL_SPI_EnableIT_RXNE(SPIx);
    LL_SPI_EnableIT_ERR(SPIx);

#ifdef STM32G0
    LL_DMA_SetPeriphRequest(DMA1, DMA_CH, LL_DMAMUX_REQ_SPIx_TX);
#endif
    LL_DMA_ConfigTransfer(DMA1, DMA_CH,
                          LL_DMA_DIRECTION_MEMORY_TO_PERIPH | //
                              LL_DMA_PRIORITY_LOW |           //
                              LL_DMA_MODE_NORMAL |            //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);

#if SPI_RX
#ifdef STM32G0
    LL_DMA_SetPeriphRequest(DMA1, DMA_CH_RX, LL_DMAMUX_REQ_SPIx_RX);
#endif
    LL_DMA_ConfigTransfer(DMA1, DMA_CH_RX,
                          LL_DMA_DIRECTION_PERIPH_TO_MEMORY | //
                              LL_DMA_PRIORITY_HIGH |          //
                              LL_DMA_MODE_NORMAL |            //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);
#endif

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TC(DMA1, DMA_CH);
    LL_DMA_EnableIT_TE(DMA1, DMA_CH);

    NVIC_SetPriority(DMA_IRQn, 1);
    NVIC_EnableIRQ(DMA_IRQn);

    NVIC_SetPriority(IRQn, 1);
    NVIC_EnableIRQ(IRQn);

    LL_SPI_Enable(SPIx);
}
