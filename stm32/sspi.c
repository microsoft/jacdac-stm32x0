#include "jdstm.h"

#include "services/interfaces/jd_pixel.h"

#define CHUNK_LEN 3 * 4 * 2
#define PX_SCRATCH_LEN (6 * CHUNK_LEN) // 144 bytes

#ifdef PIN_SSCK

#if defined(STM32G031xx) || defined(STM32G030xx)
#if PIN_SSCK == PA_5 || PIN_SSCK == PA_1
#define SPI_IDX 1
#elif PIN_SSCK == PA_0
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
#if PIN_SSCK == PA_5
#define PIN_AF LL_GPIO_AF_0
STATIC_ASSERT(PIN_SSCK == PA_5);

STATIC_ASSERT(PIN_SMOSI == PA_7 || PIN_SMOSI == PA_2);
#if SPI_RX
STATIC_ASSERT(PIN_SMISO == PA_6);
#endif
#elif PIN_SSCK == PA_1
#define PIN_AF LL_GPIO_AF_0
STATIC_ASSERT(PIN_SSCK == PA_1);
STATIC_ASSERT(PIN_SMOSI == PA_2);
#if SPI_RX
STATIC_ASSERT(PIN_SMISO == PA_6);
#endif
#else
#error "not supported"
#endif
#elif SPI_IDX == 2
#define SPI_CLK_ENABLE __HAL_RCC_SPI2_CLK_ENABLE
#if defined(STM32G031xx)
#if PIN_SSCK == PA_0
#define PIN_AF LL_GPIO_AF_0
#define PIN_AF_MOSI LL_GPIO_AF_1
STATIC_ASSERT(PIN_SSCK == PA_0);
STATIC_ASSERT(PIN_SMOSI == PB_7);
#if SPI_RX
// not implemented
STATIC_ASSERT(PIN_SMISO == -1);
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

// this is only enabled for error events
void IRQHandler(void) {
    (void)SPIx->DR;
    (void)SPIx->SR;
    // ERROR("SPI %x %x", SPIx->DR, SPIx->SR);
}

void sspi_init(void) {
    SPI_CLK_ENABLE();
    DMA_CLK_ENABLE();

    pin_setup_output_af(PIN_SSCK, PIN_AF);

// some pins require an alternate AF enum for specific pins.
#ifdef PIN_AF_MOSI
    pin_setup_output_af(PIN_SMOSI, PIN_AF_MOSI);
#else
    pin_setup_output_af(PIN_SMOSI, PIN_AF);
#endif

    pin_setup_output_af(PIN_SMISO, PIN_AF);
    LL_SPI_Disable(SPIx);

    SPIx->CR1 = LL_SPI_MODE_MASTER | LL_SPI_NSS_SOFT | LL_SPI_PHASE_2EDGE |
                LL_SPI_BAUDRATEPRESCALER_DIV16 | LL_SPI_FULL_DUPLEX | LL_SPI_POLARITY_HIGH;
    SPIx->CR2 = LL_SPI_DATAWIDTH_8BIT | LL_SPI_RX_FIFO_TH_QUARTER;

    // LL_SPI_EnableIT_TXE(SPIx);
    // LL_SPI_EnableIT_RXNE(SPIx);
    LL_SPI_EnableIT_ERR(SPIx);

    NVIC_SetPriority(IRQn, 1);
    NVIC_EnableIRQ(IRQn);
    LL_SPI_Enable(SPIx);
}

void sspi_tx(uint8_t *data, uint32_t numbytes) {
    while(numbytes--) {
        LL_SPI_TransmitData8(SPIx, (uint8_t)*(data++));
        while (LL_SPI_IsActiveFlag_TXE(SPIx) == 0);
    }
    while(LL_SPI_IsActiveFlag_BSY(SPIx) == 1);
}

void sspi_rx(uint8_t *buf, uint32_t numbytes) {
    while(numbytes--) {
        LL_SPI_TransmitData8(SPIx, 0);
        while (LL_SPI_IsActiveFlag_RXNE(SPIx) == 0);
        // while (LL_SPI_GetRxFIFOLevel(SPIx) == LL_SPI_RX_FIFO_EMPTY);
        *(buf++) = LL_SPI_ReceiveData8(SPIx);
    }
}
#endif