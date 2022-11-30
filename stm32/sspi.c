#include "jdstm.h"

#include "services/interfaces/jd_pixel.h"

#define CHUNK_LEN 3 * 4 * 2
#define PX_SCRATCH_LEN (6 * CHUNK_LEN) // 144 bytes

#ifdef PIN_SSCK

#define PIN_ASCK PIN_SSCK
#define PIN_AMOSI PIN_SMOSI

#if SPI_RX
#define PIN_AMISO PIN_SMISO
#endif

#include "spidef.h"

// this is only enabled for error events
void IRQHandler(void) {
    (void)SPIx->DR;
    (void)SPIx->SR;
    // ERROR("SPI %x %x", SPIx->DR, SPIx->SR);
}

void sspi_init(int slow, int cpol, int cpha) {
    SPI_CLK_ENABLE();

    pin_setup_output_af(PIN_SSCK, PIN_AF);

// some pins require an alternate AF enum for specific pins.
#ifdef PIN_AF_MOSI
    pin_setup_output_af(PIN_SMOSI, PIN_AF_MOSI);
#else
    pin_setup_output_af(PIN_SMOSI, PIN_AF);
#endif

#if SPI_RX
    pin_setup_output_af(PIN_SMISO, PIN_AF);
#endif

    LL_SPI_Disable(SPIx);

    uint32_t params = 0;

    if (cpol)
        params |= SPI_CR1_CPOL;

    if (cpha)
        params |= SPI_CR1_CPHA;

    if (slow)
        params |= LL_SPI_BAUDRATEPRESCALER_DIV16;

    SPIx->CR1 = LL_SPI_MODE_MASTER | LL_SPI_NSS_SOFT | LL_SPI_FULL_DUPLEX | params;
    SPIx->CR2 = LL_SPI_DATAWIDTH_8BIT | LL_SPI_RX_FIFO_TH_HALF;

    // LL_SPI_EnableIT_TXE(SPIx);
    // LL_SPI_EnableIT_RXNE(SPIx);
    LL_SPI_EnableIT_ERR(SPIx);

    NVIC_SetPriority(IRQn, IRQ_PRIORITY_DMA);
    NVIC_EnableIRQ(IRQn);
    LL_SPI_Enable(SPIx);
}

void sspi_tx(const void *data, uint32_t numbytes) {
    const uint8_t *p = data;
    while (numbytes--) {
        while (LL_SPI_IsActiveFlag_TXE(SPIx) == 0)
            ;
        LL_SPI_TransmitData8(SPIx, *p++);
    }
    while (LL_SPI_IsActiveFlag_BSY(SPIx))
        ;
}

#if SPI_RX
void sspi_rx(void *buf0, uint32_t numbytes) {
    // flush input fifo
    while (LL_SPI_GetRxFIFOLevel(SPIx) != LL_SPI_RX_FIFO_EMPTY)
        LL_SPI_ReceiveData8(SPIx);

    uint8_t *buf = buf0;
    uint32_t numbytes0 = numbytes;

    while (numbytes--) {
        while (LL_SPI_IsActiveFlag_TXE(SPIx) == 0)
            ;
        LL_SPI_TransmitData8(SPIx, 0);
        while (LL_SPI_GetRxFIFOLevel(SPIx) != LL_SPI_RX_FIFO_EMPTY)
            *(buf++) = LL_SPI_ReceiveData8(SPIx);
    }
    while (LL_SPI_IsActiveFlag_BSY(SPIx))
        ;
    while (LL_SPI_GetRxFIFOLevel(SPIx) != LL_SPI_RX_FIFO_EMPTY)
        *(buf++) = LL_SPI_ReceiveData8(SPIx);

    if (buf - (uint8_t *)buf0 != (int)numbytes0)
        JD_PANIC();
}
#endif
#endif