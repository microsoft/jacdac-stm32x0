#include "jdstm.h"

#if defined(USE_SPIS)

#include "spidef.h"

cb_t spis_halftransfer_cb;
cb_t spis_error_cb;
static cb_t doneH;

static void pulse_log_pin(void) {}

#define DMA_FLAG_GLOBAL DMA_ISR_GIF1_Pos
#define DMA_FLAG_COMPLETE DMA_ISR_TCIF1_Pos
#define DMA_FLAG_HALF DMA_ISR_HTIF1_Pos
#define DMA_FLAG_ERROR DMA_ISR_TEIF1_Pos

static inline bool DMA_HasFlag(DMA_TypeDef *DMAx, uint8_t ch, uint8_t flag) {
    return (DMAx->ISR & (1 << (4 * ch + flag))) != 0;
}

static inline void DMA_ClearFlag(DMA_TypeDef *DMAx, uint8_t ch) {
    DMAx->IFCR = 1 << (4 * ch + DMA_FLAG_GLOBAL);
}

static void spi_core_init(void) {
    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();

    SPIx->CR1 =
        LL_SPI_FULL_DUPLEX | LL_SPI_MODE_SLAVE | LL_SPI_NSS_SOFT | LL_SPI_BAUDRATEPRESCALER_DIV2;
    SPIx->CR2 = LL_SPI_DATAWIDTH_8BIT | LL_SPI_RX_FIFO_TH_QUARTER;

    // LL_SPI_EnableIT_TXE(SPIx);
    // LL_SPI_EnableIT_RXNE(SPIx);
    LL_SPI_EnableIT_ERR(SPIx);
}

bool spis_seems_connected() {
    return pin_get(PIN_ASCK) == 0;
}

void spis_init() {
    SPI_CLK_ENABLE();
    DMA_CLK_ENABLE();

    pin_setup_output_af(PIN_ASCK, PIN_AF);
    pin_setup_output_af(PIN_AMISO, PIN_AF);
    pin_setup_output_af(PIN_AMOSI, PIN_AF);

    spi_core_init();

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TE(DMA1, DMA_CH_TX);
    LL_DMA_EnableIT_TC(DMA1, DMA_CH_RX);
    LL_DMA_EnableIT_TE(DMA1, DMA_CH_RX);
    LL_DMA_EnableIT_HT(DMA1, DMA_CH_RX);

    spi_init1();
}

void spis_xfer(const void *txdata, void *rxdata, uint32_t numbytes, cb_t doneHandler) {
    // DMESG("xfer %x", SPIx->SR);

    if (doneH)
        JD_PANIC();
    doneH = doneHandler;

    /* Reset the threshold bit */
    CLEAR_BIT(SPIx->CR2, SPI_CR2_LDMATX | SPI_CR2_LDMARX);

    LL_DMA_ConfigAddresses(DMA1, DMA_CH_RX, (uint32_t) & (SPIx->DR), (uint32_t)rxdata,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, DMA_CH_RX, numbytes);

    LL_DMA_ConfigAddresses(DMA1, DMA_CH_TX, (uint32_t)txdata, (uint32_t) & (SPIx->DR),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, DMA_CH_TX, numbytes);

    LL_SPI_EnableDMAReq_RX(SPIx);
    LL_DMA_EnableChannel(DMA1, DMA_CH_RX);
    LL_DMA_EnableChannel(DMA1, DMA_CH_TX);
    LL_SPI_EnableDMAReq_TX(SPIx);
    LL_SPI_Enable(SPIx);

    pulse_log_pin();
    // DMESG("xfer2 %x", SPIx->SR);
}

void spis_log() {
    DMESG("%d %x %d/%d", (int)tim_get_micros(), (unsigned)SPIx->SR,
          (int)LL_DMA_GetDataLength(DMA1, DMA_CH_TX), (int)LL_DMA_GetDataLength(DMA1, DMA_CH_RX));
}

static void shutdown_dma(void) {
    DMA_ClearFlag(DMA1, DMA_CH_RX);
    DMA_ClearFlag(DMA1, DMA_CH_TX);
    LL_DMA_DisableChannel(DMA1, DMA_CH_TX);
    LL_DMA_DisableChannel(DMA1, DMA_CH_RX);
    LL_SPI_Disable(SPIx);
}

// this is only enabled for error events
void IRQHandler(void) {
    DMESG("SPI-S handler! %x %d/%d", (unsigned)SPIx->SR, (int)LL_DMA_GetDataLength(DMA1, DMA_CH_TX),
          (int)LL_DMA_GetDataLength(DMA1, DMA_CH_RX));
    if (!spis_error_cb)
        JD_PANIC();
    shutdown_dma();
    spis_error_cb();
}

// static int n;

void DMA_Handler(void) {
    // uint32_t isr = DMA1->ISR;
    // DMESG("DMA-SPIS irq %x", DMA1->ISR);

    //    if (n++ > 10)
    //        JD_PANIC();

    pulse_log_pin();

    if (DMA_HasFlag(DMA1, DMA_CH_RX, DMA_FLAG_COMPLETE)) {
        pulse_log_pin();
#if 0
        spis_log();
        JD_PANIC();
#endif
        shutdown_dma();

        cb_t f = doneH;
        if (f) {
            doneH = NULL;
            f();
        }
    } else if (DMA_HasFlag(DMA1, DMA_CH_RX, DMA_FLAG_HALF)) {
        DMA_ClearFlag(DMA1, DMA_CH_RX);
        // DMESG("DMA-SPIS-cl irq %x", DMA1->ISR);
        // set_log_pin2(1);
        // set_log_pin2(0);
        if (spis_halftransfer_cb)
            spis_halftransfer_cb();
    }

    if (DMA_HasFlag(DMA1, DMA_CH_RX, DMA_FLAG_ERROR) ||
        DMA_HasFlag(DMA1, DMA_CH_TX, DMA_FLAG_ERROR)) {
        JD_PANIC();
    }
}

void spis_abort() {
    doneH = NULL;
    shutdown_dma();
    spi_core_init();
}

#endif