#include "jdsimple.h"

#define DMA_FLAG_GLOBAL DMA_ISR_GIF1_Pos
#define DMA_FLAG_COMPLETE DMA_ISR_TCIF1_Pos
#define DMA_FLAG_HALF DMA_ISR_HTIF1_Pos
#define DMA_FLAG_ERROR DMA_ISR_TEIF1_Pos

static inline bool DMA_HasFlag(DMA_TypeDef *DMAx, uint8_t ch, uint8_t flag) {
    return (DMAx->ISR & (1 << (4 * (ch - 1) + flag))) != 0;
}

static inline void DMA_ClearFlag(DMA_TypeDef *DMAx, uint8_t ch) {
    DMAx->IFCR = 1 << (4 * (ch - 1) + DMA_FLAG_GLOBAL);
}

#define SPI_IDX 1

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
#define PIN_PORT GPIOA
#define PIN_SCK LL_GPIO_PIN_5
#define PIN_MOSI LL_GPIO_PIN_7
#define PIN_MISO LL_GPIO_PIN_6
#define PIN_CS LL_GPIO_PIN_4
#define PIN_AF LL_GPIO_AF_0
#define SPI_CLK_ENABLE __HAL_RCC_SPI1_CLK_ENABLE
#elif SPI_IDX == 2
#error "not finished"
#define PIN_PORT GPIOB
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
#else
#error "bad spi"
#endif

#define DMA_CH_RX LL_DMA_CHANNEL_2
#define DMA_CH_TX LL_DMA_CHANNEL_3

#define DMA_IRQn DMA1_Channel2_3_IRQn
#define DMA_Handler DMA1_Channel2_3_IRQHandler

cb_t spis_halftransfer_cb;
cb_t spis_error_cb;
static cb_t doneH;

static void spi_core_init() {
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
    return LL_GPIO_IsInputPinSet(PIN_PORT, PIN_SCK) == 0;
}

void spis_init() {
    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = PIN_SCK | PIN_MOSI | PIN_MISO;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP; // to detect host removal
    GPIO_InitStruct.Alternate = PIN_AF;
    LL_GPIO_Init(PIN_PORT, &GPIO_InitStruct);

    spi_core_init();

#ifdef STM32G0
    LL_DMA_SetPeriphRequest(DMA1, DMA_CH_TX, LL_DMAMUX_REQ_SPIx_TX);
#endif
    LL_DMA_ConfigTransfer(DMA1, DMA_CH_TX,
                          LL_DMA_DIRECTION_MEMORY_TO_PERIPH | //
                              LL_DMA_PRIORITY_MEDIUM |        //
                              LL_DMA_MODE_NORMAL |            //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);

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

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TE(DMA1, DMA_CH_TX);
    LL_DMA_EnableIT_TC(DMA1, DMA_CH_RX);
    LL_DMA_EnableIT_TE(DMA1, DMA_CH_RX);
    LL_DMA_EnableIT_HT(DMA1, DMA_CH_RX);

    NVIC_SetPriority(DMA_IRQn, 1);
    NVIC_EnableIRQ(DMA_IRQn);

    NVIC_SetPriority(IRQn, 1);
    NVIC_EnableIRQ(IRQn);
}

void spis_xfer(const void *txdata, void *rxdata, uint32_t numbytes, cb_t doneHandler) {
    // DMESG("xfer %x", SPIx->SR);

    if (doneH)
        panic();
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
    DMESG("%d %x %d/%d", (int)tim_get_micros(), SPIx->SR, LL_DMA_GetDataLength(DMA1, DMA_CH_TX),
          LL_DMA_GetDataLength(DMA1, DMA_CH_RX));
}

static void shutdown_dma() {
    DMA_ClearFlag(DMA1, DMA_CH_RX);
    DMA_ClearFlag(DMA1, DMA_CH_TX);
    LL_DMA_DisableChannel(DMA1, DMA_CH_TX);
    LL_DMA_DisableChannel(DMA1, DMA_CH_RX);
    LL_SPI_Disable(SPIx);
}

// this is only enabled for error events
void IRQHandler(void) {
    DMESG("SPI-S handler! %x %d/%d", SPIx->SR, LL_DMA_GetDataLength(DMA1, DMA_CH_TX),
          LL_DMA_GetDataLength(DMA1, DMA_CH_RX));
    if (!spis_error_cb)
        panic();
    shutdown_dma();
    spis_error_cb();
}

// static int n;

void DMA_Handler(void) {
    // uint32_t isr = DMA1->ISR;
    // DMESG("DMA-SPIS irq %x", DMA1->ISR);

    //    if (n++ > 10)
    //        panic();

    pulse_log_pin();

    if (DMA_HasFlag(DMA1, DMA_CH_RX, DMA_FLAG_COMPLETE)) {
        pulse_log_pin();
#if 0
        spis_log();
        panic();
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
        panic();
    }
}

void spis_abort() {
    doneH = NULL;
    shutdown_dma();
    spi_core_init();
}