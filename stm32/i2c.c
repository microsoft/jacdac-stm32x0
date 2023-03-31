#include "jdstm.h"

#ifdef PIN_SDA

#ifndef I2C_IDX
#define I2C_IDX 1
#endif

#if I2C_IDX == 1
#define I2Cx I2C1
#define I2C_CLK LL_APB1_GRP1_PERIPH_I2C1
#define I2C_CLK_SRC LL_RCC_I2C1_CLKSOURCE_HSI
#elif I2C_IDX == 2
#define I2Cx I2C2
#define I2C_CLK LL_APB1_GRP1_PERIPH_I2C2
// only G0Bx and G0Cx support this - we don't really want PCLK source, so in reality only I2C1 is
// supported
#define I2C_CLK_SRC LL_RCC_I2C2_CLKSOURCE_HSI
#endif

#ifndef I2C_FAST_MODE
#define I2C_FAST_MODE 1
#endif

// I2C_TIMINGR
// 0x3     0   4      2      0F   13
//   PRESC RES SCLDEL SDADEL SCLH SCLL

#ifdef STM32F0
#if 1
// 22.4.10 I2C_TIMINGR register configuration examples
#if I2C_FAST_MODE
#define I2C_TIMING 0x00310309
#else
#define I2C_TIMING 0x00420F13
#endif
#else
// rise 100ns, fall 10ns
#if I2C_FAST_MODE
#define I2C_TIMING 0x0010020A
#else
#define I2C_TIMING 0x00201D2B
#endif
#endif
#endif

#if defined(STM32G0) || defined(STM32L)
// 25.4.11 I2C_TIMINGR register configuration examples (G0 docs)
// 32.4.10 in WL docs
#if I2C_FAST_MODE
#define I2C_TIMING 0x10320309
#else
#define I2C_TIMING 0x30420F13
#endif
#endif

static void setup_pin(uint8_t pin) {
    pin_setup_output_af(pin, I2C_AF);
    LL_GPIO_SetPinOutputType(PIN_PORT(pin), PIN_MASK(pin), LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinPull(PIN_PORT(pin), PIN_MASK(pin), LL_GPIO_PULL_UP);
}

int i2c_init_(void) {
    setup_pin(PIN_SDA);
    setup_pin(PIN_SCL);
    LL_RCC_SetI2CClockSource(I2C_CLK_SRC);
    LL_APB1_GRP1_EnableClock(I2C_CLK);
    LL_I2C_Disable(I2Cx);
    LL_I2C_SetTiming(I2Cx, I2C_TIMING);
    LL_I2C_Enable(I2Cx);
    return 0;
}

#ifdef STM32F0
#define CYCLES_PER_MS (77 * cpu_mhz)
#elif defined(STM32G0)
#define CYCLES_PER_MS (100 * cpu_mhz) // TODO measure this!
#elif defined(STM32L)
#define CYCLES_PER_MS (100 * cpu_mhz) // TODO measure this!
#else
#error "measure CYCLES_PER_MS"
#endif

static int wait_for_ack(void) {
    unsigned k = 10 * CYCLES_PER_MS;
    while (k--) {
        if (LL_I2C_IsActiveFlag_TXIS(I2Cx))
            return 0;
    }
    return -1;
}

int i2c_setup_write(uint8_t addr, unsigned len, bool repeated) {
    addr <<= 1;
    LL_I2C_HandleTransfer(I2Cx, addr, LL_I2C_ADDRSLAVE_7BIT, len,
                          repeated ? LL_I2C_MODE_SOFTEND : LL_I2C_MODE_AUTOEND,
                          LL_I2C_GENERATE_START_WRITE);

    if (wait_for_ack()) {
        DMESG("nack %x", addr);
        return -10;
    }

    return 0;
}

int i2c_write(uint8_t c) {
    LL_I2C_TransmitData8(I2Cx, c);

    unsigned k = 3 * CYCLES_PER_MS;
    while (k--) {
        if (LL_I2C_IsActiveFlag_TXE(I2Cx)) {
            if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
                DMESG("nack wr:%x", c);
                return -12;
            } else {
                return 0;
            }
        }
    }

    DMESG("t/o wr:%x", c);
    return -13;
}

int i2c_finish_write(bool repeated) {
    if (repeated) {
        while (!LL_I2C_IsActiveFlag_TC(I2Cx))
            ;
    } else {
        while (!LL_I2C_IsActiveFlag_STOP(I2Cx))
            ;
        LL_I2C_ClearFlag_STOP(I2Cx);
    }
    return 0;
}

#define CHECK_RET(call)                                                                            \
    do {                                                                                           \
        int _r = call;                                                                             \
        if (_r < 0)                                                                                \
            return _r;                                                                             \
    } while (0)

#define MAXREP 10000

int i2c_read_ex(uint8_t addr, void *dst, unsigned len) {
    addr <<= 1;
    LL_I2C_HandleTransfer(I2Cx, addr, LL_I2C_ADDRSLAVE_7BIT, len, LL_I2C_MODE_AUTOEND,
                          LL_I2C_GENERATE_RESTART_7BIT_READ);
    uint8_t *p = dst;
    uint8_t *end = p + len;
    while (p < end) {
        unsigned i;
        for (i = 0; i < MAXREP; ++i)
            if (LL_I2C_IsActiveFlag_RXNE(I2Cx))
                break;

        if (i >= MAXREP) {
            DMESG("T/o rd");
            return -1;
        }

        *p++ = LL_I2C_ReceiveData8(I2Cx);
    }

    while (!LL_I2C_IsActiveFlag_STOP(I2Cx))
        ;
    LL_I2C_ClearFlag_STOP(I2Cx);

    return 0;
}

int i2c_write_ex2(uint8_t device_address, const void *src, unsigned len, const void *src2,
                  unsigned len2, bool repeated)

{
    CHECK_RET(i2c_setup_write(device_address, len + len2, repeated));
    const uint8_t *p = src;
    while (len--)
        CHECK_RET(i2c_write(*p++));
    p = src2;
    while (len2--)
        CHECK_RET(i2c_write(*p++));
    CHECK_RET(i2c_finish_write(repeated));
    return 0;
}

#endif