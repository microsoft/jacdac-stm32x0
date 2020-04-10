#include "jdstm.h"

#define MASK_SET(p) (1 << ((p)&0xf))
#define MASK_CLR(p) (1 << (((p)&0xf) + 16))

// at 48MHz
// 1 is around 1MHz - still works up to 1.25MHz or so
// 5 is 660kHz
// 10 is 430kHz
#define NOP target_wait_cycles(5)

#define SEND_BIT(n)                                                                                \
    NOP;                                                                                           \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_MOSI) | (((b >> n) & 1) << (PIN_ACC_MOSI & 0xf));            \
    ACC_PORT->BSRR = MASK_SET(PIN_ACC_SCK);                                                        \
    NOP;                                                                                           \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_SCK)

#define RECV_BIT(n)                                                                                \
    NOP;                                                                                           \
    ACC_PORT->BSRR = MASK_SET(PIN_ACC_SCK);                                                        \
    NOP;                                                                                           \
    b |= ((ACC_PORT->IDR >> (PIN_ACC_MISO & 0xf)) & 1) << n;                                       \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_SCK)

void bspi_send(const void *src, uint32_t len) {
    // target_wait_us(5);
    for (int i = 0; i < len; ++i) {
        uint8_t b = ((uint8_t*)src)[i];
        SEND_BIT(7);
        SEND_BIT(6);
        SEND_BIT(5);
        SEND_BIT(4);
        SEND_BIT(3);
        SEND_BIT(2);
        SEND_BIT(1);
        SEND_BIT(0);
    }
    pin_set(PIN_ACC_MOSI, 0);
}

void bspi_recv(void *dst, uint32_t len) {
    // target_wait_us(5);
    for (int i = 0; i < len; ++i) {
        uint8_t b = 0x00;
        RECV_BIT(7);
        RECV_BIT(6);
        RECV_BIT(5);
        RECV_BIT(4);
        RECV_BIT(3);
        RECV_BIT(2);
        RECV_BIT(1);
        RECV_BIT(0);
        ((uint8_t*)dst)[i] = b;
    }
}
