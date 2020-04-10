#include "jdsimple.h"

#ifdef STM32F031x6
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
#define PIN_PORT GPIOA
#define PIN_SCK LL_GPIO_PIN_5
#define PIN_MOSI LL_GPIO_PIN_7
#define PIN_AF LL_GPIO_AF_0
#define SPI_CLK_ENABLE __HAL_RCC_SPI1_CLK_ENABLE
#elif SPI_IDX == 2
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

#ifdef STM32G0
#define DMA_CH LL_DMA_CHANNEL_1
#define DMA_IRQn DMA1_Channel1_IRQn
#define DMA_Handler DMA1_Channel1_IRQHandler
#define DMA_ClearFlag LL_DMA_ClearFlag_GI1
#else

#if SPI_IDX == 1
#define DMA_CH LL_DMA_CHANNEL_3
#define DMA_IRQn DMA1_Channel2_3_IRQn
#define DMA_Handler DMA1_Channel2_3_IRQHandler
#define DMA_ClearFlag LL_DMA_ClearFlag_GI3
#elif SPI_IDX == 2
#define DMA_CH LL_DMA_CHANNEL_5
#define DMA_IRQn DMA1_Channel4_5_IRQn
#define DMA_Handler DMA1_Channel4_5_IRQHandler
#define DMA_ClearFlag LL_DMA_ClearFlag_GI5
#else
#error "bad spi"
#endif

#endif

void dspi_init() {
    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = PIN_SCK | PIN_MOSI;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Alternate = PIN_AF;
    LL_GPIO_Init(PIN_PORT, &GPIO_InitStruct);

    SPIx->CR1 = LL_SPI_HALF_DUPLEX_TX | LL_SPI_MODE_MASTER | LL_SPI_NSS_SOFT |
                LL_SPI_BAUDRATEPRESCALER_DIV2;
    SPIx->CR2 = LL_SPI_DATAWIDTH_8BIT | LL_SPI_RX_FIFO_TH_QUARTER;

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

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TC(DMA1, DMA_CH);
    LL_DMA_EnableIT_TE(DMA1, DMA_CH);

    NVIC_SetPriority(DMA_IRQn, 1);
    NVIC_EnableIRQ(DMA_IRQn);

    NVIC_SetPriority(IRQn, 1);
    NVIC_EnableIRQ(IRQn);
}

static cb_t doneH;

static void tx_core(const void *data, uint32_t numbytes, cb_t doneHandler) {
    // DMESG("dspi tx");

    if (doneH)
        jd_panic();
    doneH = doneHandler;

    /* Reset the threshold bit */
    CLEAR_BIT(SPIx->CR2, SPI_CR2_LDMATX | SPI_CR2_LDMARX);

    LL_DMA_ConfigAddresses(DMA1, DMA_CH, (uint32_t)data, (uint32_t) & (SPIx->DR),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, DMA_CH, numbytes);

    LL_SPI_EnableDMAReq_TX(SPIx);
}

void dspi_tx(const void *data, uint32_t numbytes, cb_t doneHandler) {
    tx_core(data, numbytes, doneHandler);
    LL_SPI_Enable(SPIx);
    LL_DMA_EnableChannel(DMA1, DMA_CH);
}

void px_tx(const void *data, uint32_t numbytes, cb_t doneHandler) {
    tx_core(data, numbytes >> 1, doneHandler);
    LL_I2S_Enable(SPIx);
    LL_DMA_EnableChannel(DMA1, DMA_CH);
}

static uint16_t pxlookup[16];
static void init_lookup() {
    for (int i = 0; i < 16; ++i) {
        uint16_t v = 0;
        for (int mask = 0x8; mask > 0; mask >>= 1) {
            v <<= 3;
            if (i & mask)
                v |= 6;
            else
                v |= 4;
        }
        pxlookup[i] = v;
    }
}

static inline void set_byte(uint8_t *dst, uint8_t v) {
    uint32_t q = (pxlookup[v >> 4] << 12) | pxlookup[v & 0xf];
    if ((uint32_t)dst & 1) {
        dst[-1] = q >> 16;
        dst[2] = q >> 8;
        dst[1] = q >> 0;
    } else {
        dst[1] = q >> 16;
        dst[0] = q >> 8;
        dst[3] = q >> 0;
    }
}

#define SCALE(c, i) ((((c)&0xff) * (i & 0xff)) >> 8)
void px_set(const void *data, uint32_t index, uint8_t intensity, uint32_t color) {
    uint8_t *dst = (uint8_t *)data + index * 9;
    // assume GRB
    set_byte(dst, SCALE(color >> 8, intensity));
    set_byte(dst + 3, SCALE(color >> 0, intensity));
    set_byte(dst + 6, SCALE(color >> 16, intensity));
}

// this is only enabled for error events
void IRQHandler(void) {
    DMESG("SPI handler!");
}

void DMA_Handler(void) {
    // uint32_t isr = DMA1->ISR;

    DMA_ClearFlag(DMA1);
    LL_DMA_DisableChannel(DMA1, DMA_CH);

    // DMESG("DMA irq %x", isr);

    while (LL_SPI_GetTxFIFOLevel(SPIx))
        ;
    while (LL_SPI_IsActiveFlag_BSY(SPIx))
        ;

    // DMESG("DMA spi done waiting");

    cb_t f = doneH;
    if (f) {
        doneH = NULL;
        f();
    }
}

void px_init() {
    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = PIN_MOSI;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Alternate = PIN_AF;
    LL_GPIO_Init(PIN_PORT, &GPIO_InitStruct);

    SPIx->I2SPR = 10;
    SPIx->I2SCFGR = SPI_I2SCFGR_I2SMOD | LL_I2S_MODE_MASTER_TX;
    SPIx->CR1 = 0;
    SPIx->CR2 = 0;

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
                              LL_DMA_PDATAALIGN_HALFWORD |    //
                              LL_DMA_MDATAALIGN_HALFWORD);

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TC(DMA1, DMA_CH);
    LL_DMA_EnableIT_TE(DMA1, DMA_CH);

    NVIC_SetPriority(DMA_IRQn, 1);
    NVIC_EnableIRQ(DMA_IRQn);

    NVIC_SetPriority(IRQn, 1);
    NVIC_EnableIRQ(IRQn);

    init_lookup();
}
