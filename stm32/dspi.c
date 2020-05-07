#include "jdstm.h"

#ifdef PIN_ASCK

#if defined(STM32F031x6) || !defined(SPI2)
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
#define PIN_AF LL_GPIO_AF_0
STATIC_ASSERT(PIN_ASCK == PA_5);
STATIC_ASSERT(PIN_AMOSI == PA_7);
#define SPI_CLK_ENABLE __HAL_RCC_SPI1_CLK_ENABLE
#elif SPI_IDX == 2
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
#else

#if SPI_IDX == 1
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

static inline void dma_clear_flag(int flag) {
    WRITE_REG(DMA1->IFCR, flag << ((DMA_CH - 1) * 4));
}

static inline bool dma_has_flag(int flag) {
    return (READ_BIT(DMA1->ISR, flag << ((DMA_CH - 1) * 4)) != 0);
}

#define PX_SCRATCH_LEN 128

typedef struct px_state {
    uint16_t pxlookup[16];
    uint16_t *pxscratch;
    const uint8_t *pxdata;
    uint16_t pxdata_len;
    uint16_t pxdata_ptr;
} px_state_t;
static px_state_t px_state;

static cb_t doneH, dma_handler;

void dspi_init() {
    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    pin_setup_output_af(PIN_ASCK, PIN_AF);
    pin_setup_output_af(PIN_AMOSI, PIN_AF);

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

static void px_fill_buffer(uint16_t *dst) {
    unsigned numbytes = PX_SCRATCH_LEN / 2 / 4;
    unsigned start = px_state.pxdata_ptr;
    unsigned end = start + numbytes;

    if (end >= px_state.pxdata_len) {
        // just zero-out the whole thing, so we don't get junk at the end
        memset(dst, 0, PX_SCRATCH_LEN / 2);
        end = px_state.pxdata_len;
        if (px_state.pxdata_ptr == end)
            px_state.pxdata_ptr = 0;
        else
            px_state.pxdata_ptr = end; // we do one more round, with an empty buffer
    } else {
        px_state.pxdata_ptr = end;
    }
    while (start < end) {
        uint8_t c = px_state.pxdata[start++];
        *dst++ = px_state.pxlookup[c >> 4];
        *dst++ = px_state.pxlookup[c & 0xf];
    }
}

static void px_dma(void) {
    if (dma_has_flag(DMA_FLAG_TC)) {
        dma_clear_flag(DMA_FLAG_TC);
        px_fill_buffer(px_state.pxscratch + (PX_SCRATCH_LEN >> 2));
    }
    if (dma_has_flag(DMA_FLAG_HT)) {
        dma_clear_flag(DMA_FLAG_HT);
        px_fill_buffer(px_state.pxscratch);
    }
    dma_clear_flag(DMA_FLAG_G);
    if (px_state.pxdata_ptr == 0 && doneH) {
        cb_t f = doneH;
        doneH = NULL;
        f();
    }
}

void px_tx(const void *data, uint32_t numbytes, cb_t doneHandler) {
    int firstTime = px_state.pxdata == NULL;
    px_state.pxdata = data;
    px_state.pxdata_len = numbytes;
    if (firstTime) {
        pwr_enter_pll();
        px_state.pxscratch = alloc(PX_SCRATCH_LEN);
        px_state.pxdata_ptr = 0;
        dma_clear_flag(DMA_FLAG_TC);
        dma_clear_flag(DMA_FLAG_HT);
        px_fill_buffer(px_state.pxscratch);
        px_fill_buffer(px_state.pxscratch + (PX_SCRATCH_LEN >> 2));
        dma_handler = px_dma;

        tx_core(px_state.pxscratch, PX_SCRATCH_LEN, NULL);
        LL_SPI_Enable(SPIx);
        LL_DMA_EnableChannel(DMA1, DMA_CH);
    }
    doneH = doneHandler;
}

static void init_lookup(void) {
    for (int i = 0; i < 16; ++i) {
        uint16_t v = 0;
        for (int mask = 0x8; mask > 0; mask >>= 1) {
            v <<= 4;
            if (i & mask)
                v |= 0xe;
            else
                v |= 0x8;
        }
        px_state.pxlookup[i] = (v >> 8) | (v << 8);
    }
}

#define SCALE(c, i) ((((c)&0xff) * (1 + (i & 0xff))) >> 8)
void px_set(const void *data, uint32_t index, uint8_t intensity, uint32_t color) {
    uint8_t *dst = (uint8_t *)data + index * 3;
    // assume GRB
    dst[0] = SCALE(color >> 8, intensity);
    dst[1] = SCALE(color >> 0, intensity);
    dst[2] = SCALE(color >> 16, intensity);
}

// this is only enabled for error events
void IRQHandler(void) {
    DMESG("SPI handler!");
}

void DMA_Handler(void) {
    if (dma_handler) {
        dma_handler();
        return;
    }

    dma_clear_flag(DMA_FLAG_G);
    LL_DMA_DisableChannel(DMA1, DMA_CH);

    while (LL_SPI_GetTxFIFOLevel(SPIx))
        ;
    while (LL_SPI_IsActiveFlag_BSY(SPIx))
        ;

    cb_t f = doneH;
    if (f) {
        doneH = NULL;
        f();
    }
}

// WS2812B timings, +-0.15us
// 0 - 0.40us hi 0.85us low
// 1 - 0.80us hi 0.45us low

void px_init() {

    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    pin_setup_output_af(PIN_AMOSI, PIN_AF);

#ifdef STM32G0
    LL_DMA_SetPeriphRequest(DMA1, DMA_CH, LL_DMAMUX_REQ_SPIx_TX);
#endif

#if PLL_MHZ == 48
    SPIx->CR1 = LL_SPI_HALF_DUPLEX_TX | LL_SPI_MODE_MASTER | LL_SPI_NSS_SOFT |
                LL_SPI_BAUDRATEPRESCALER_DIV16; // 3MHz
    SPIx->CR2 = LL_SPI_DATAWIDTH_8BIT | LL_SPI_RX_FIFO_TH_QUARTER;
#else
#error "define prescaler"
#endif
    LL_DMA_ConfigTransfer(DMA1, DMA_CH,
                          LL_DMA_DIRECTION_MEMORY_TO_PERIPH | //
                              LL_DMA_PRIORITY_LOW |           //
                              LL_DMA_MODE_CIRCULAR |          //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);

    // LL_SPI_EnableIT_TXE(SPIx);
    // LL_SPI_EnableIT_RXNE(SPIx);
    LL_SPI_EnableIT_ERR(SPIx);

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TC(DMA1, DMA_CH);
    LL_DMA_EnableIT_TE(DMA1, DMA_CH);
    LL_DMA_EnableIT_HT(DMA1, DMA_CH);

    NVIC_SetPriority(DMA_IRQn, 1);
    NVIC_EnableIRQ(DMA_IRQn);

    NVIC_SetPriority(IRQn, 1);
    NVIC_EnableIRQ(IRQn);

    init_lookup();
}

#endif