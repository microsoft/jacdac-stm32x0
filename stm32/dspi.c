#include "jdstm.h"

#include "services/interfaces/jd_pixel.h"

#define CHUNK_LEN 3 * 4 * 2
#define PX_SCRATCH_LEN (6 * CHUNK_LEN) // 144 bytes

#ifdef PIN_ASCK

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

typedef struct px_state {
    uint16_t pxlookup[16];
    uint16_t *pxscratch;
    const uint8_t *pxdata;
    uint16_t pxdata_len;
    uint16_t pxdata_ptr;
    uint8_t intensity;
    uint8_t type;
} px_state_t;
static px_state_t px_state;

static cb_t doneH, dma_handler;

void dspi_init(bool slow, int cpol, int cpha) {
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

    uint32_t cpol_cpha = 0;

    if (cpol)
        cpol_cpha |= LL_SPI_POLARITY_HIGH;
    
    if (cpha)
        cpol_cpha |= LL_SPI_PHASE_2EDGE;

#if SPI_RX
    pin_setup_output_af(PIN_AMISO, PIN_AF);
    uint32_t baud = LL_SPI_BAUDRATEPRESCALER_DIV4;
#else 
    uint32_t baud = LL_SPI_BAUDRATEPRESCALER_DIV2;
#endif

    if (slow)
        baud = LL_SPI_BAUDRATEPRESCALER_DIV64;

    SPIx->CR1 = LL_SPI_MODE_MASTER | LL_SPI_NSS_SOFT | cpol_cpha |
#if SPI_RX
                baud | LL_SPI_FULL_DUPLEX
#else
                baud | LL_SPI_HALF_DUPLEX_TX
#endif
        ;
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

static void stop_dma(void) {
    LL_DMA_DisableChannel(DMA1, DMA_CH);

    while (LL_SPI_GetTxFIFOLevel(SPIx))
        ;
    while (LL_SPI_IsActiveFlag_BSY(SPIx))
        ;

#if SPI_RX
    LL_DMA_DisableChannel(DMA1, DMA_CH_RX);
#endif

    cb_t f = doneH;
    if (f) {
        doneH = NULL;
        f();
    }
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

#if SPI_RX
void dspi_xfer(const void *data, void *rx, uint32_t numbytes, cb_t doneHandler) {
    if (rx && !data)
        data = rx;
    if (!rx)
        jd_panic();
    tx_core(data, numbytes, doneHandler);

    LL_DMA_ConfigAddresses(DMA1, DMA_CH_RX, (uint32_t) & (SPIx->DR), (uint32_t)rx,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, DMA_CH_RX, numbytes);
    LL_SPI_EnableDMAReq_RX(SPIx);

    LL_DMA_EnableChannel(DMA1, DMA_CH_RX);
    LL_DMA_EnableChannel(DMA1, DMA_CH);
    LL_SPI_Enable(SPIx);
}
#endif

#define SCALE(c, i) ((((c)&0xff) * (1 + (i & 0xff))) >> 8)

// for APA102 end-frame can be zero or ones, doesn't matter - we use zero in case the strip is
// longer than expected; end frame needs to be 0.5*numPixels bits long
// https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/

// brightness setting on APA is modulated at ~500Hz which can give flicker,
// while color settings are 32x higher - https://cpldcpu.wordpress.com/2014/08/27/apa102/

// however, for SK9822 the end frame has to be zero, and 32+0.5*numPixels bits long
// moreover, the current is moduled with the brightness settings
// https://cpldcpu.wordpress.com/2016/12/13/sk9822-a-clone-of-the-apa102/

static void px_fill_buffer(uint16_t *dst) {
    unsigned numbytes = PX_SCRATCH_LEN / 2 / 4;
    unsigned start = px_state.pxdata_ptr;
    if (px_state.type & LIGHT_TYPE_APA_MASK) {
        // account for start frame
        if (start == 0) {
            numbytes--;
            *(uint32_t *)dst = 0;
            dst += 2;
        }
        numbytes *= 3;
    }
    unsigned end = start + numbytes;

    if (end >= px_state.pxdata_len) {
        // just zero-out the whole thing, so we don't get junk at the end
        memset(dst, 0, PX_SCRATCH_LEN / 2);
        end = px_state.pxdata_len;
        int past = px_state.pxdata_ptr - end;
        if (past > 3)
            px_state.pxdata_ptr = 0;
        else if (past >= 0)
            px_state.pxdata_ptr++;
        else
            px_state.pxdata_ptr = end;
    } else {
        px_state.pxdata_ptr = end;
    }

    uint8_t inten = px_state.intensity;
    uint8_t apahigh = 0;

    if (px_state.type == LIGHT_TYPE_SK9822) {
        int lev = ((inten * 7967) >> 16) + 1;
        int ni = inten * 31 / lev;
        if (ni > 255)
            ni = 255;
        inten = ni;
        apahigh = 0xe0 | lev;
    } else if (px_state.type & LIGHT_TYPE_APA_MASK) {
        apahigh = 0xff;
    }

    while (start < end) {
        uint8_t r = SCALE(px_state.pxdata[start++], inten);
        uint8_t g = SCALE(px_state.pxdata[start++], inten);
        uint8_t b = SCALE(px_state.pxdata[start++], inten);
        if (apahigh) {
            *(uint32_t *)dst = (apahigh << 0) | (b << 8) | (g << 16) | (r << 24);
            dst += 2;
        } else {
            *dst++ = px_state.pxlookup[g >> 4];
            *dst++ = px_state.pxlookup[g & 0xf];
            *dst++ = px_state.pxlookup[r >> 4];
            *dst++ = px_state.pxlookup[r & 0xf];
            *dst++ = px_state.pxlookup[b >> 4];
            *dst++ = px_state.pxlookup[b & 0xf];
        }
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
    if (px_state.pxdata_ptr == 0) {
        stop_dma();
    }
}

void px_tx(const void *data, uint32_t numbytes, uint8_t intensity, cb_t doneHandler) {
    px_state.pxdata = data;
    px_state.pxdata_len = numbytes;
    px_state.pxdata_ptr = 0;
    px_state.intensity = intensity;

    dma_clear_flag(DMA_FLAG_TC);
    dma_clear_flag(DMA_FLAG_HT);
    px_fill_buffer(px_state.pxscratch);
    px_fill_buffer(px_state.pxscratch + (PX_SCRATCH_LEN >> 2));

    tx_core(px_state.pxscratch, PX_SCRATCH_LEN, doneHandler);
    LL_SPI_Enable(SPIx);
    LL_DMA_EnableChannel(DMA1, DMA_CH);
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

// this is only enabled for error events
void IRQHandler(void) {
    ERROR("SPI %x %x", SPIx->DR, SPIx->SR);
}

void DMA_Handler(void) {
    if (dma_handler) {
        dma_handler();
        return;
    }
    dma_clear_flag(DMA_FLAG_G);
    stop_dma();
}

// WS2812B timings, +-0.15us
// 0 - 0.40us hi 0.85us low
// 1 - 0.80us hi 0.45us low

void px_alloc() {
    if (px_state.pxscratch == NULL) {
        px_state.pxscratch = jd_alloc(PX_SCRATCH_LEN);
        dma_handler = px_dma;
    }
}

void px_init(int light_type) {
    SPI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    px_state.type = light_type;

    // some pins require an alternate AF enum for specific pins.
#ifdef PIN_AF_MOSI
    pin_setup_output_af(PIN_AMOSI, PIN_AF_MOSI);
#else
    pin_setup_output_af(PIN_AMOSI, PIN_AF);
#endif

    if (light_type & LIGHT_TYPE_APA_MASK)
        pin_setup_output_af(PIN_ASCK, PIN_AF);

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
    px_alloc();
}

#endif