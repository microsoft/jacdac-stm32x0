#include "jdstm.h"

#define MASK_SET(p) (1 << ((p)&0xf))
#define MASK_CLR(p) (1 << (((p)&0xf) + 16))

#ifdef LOW_POWER
#define NOP0 ((void)0)
#define NOP1 asm volatile("nop \n nop \n nop")
#else
#define NOP0 target_wait_cycles(1)
#define NOP1 target_wait_cycles(5)
#endif

#define SEND_BIT(n, wait0, wait1)                                                                  \
    wait0;                                                                                         \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_MOSI) | (((b >> n) & 1) << (PIN_ACC_MOSI & 0xf));            \
    ACC_PORT->BSRR = MASK_SET(PIN_ACC_SCK);                                                        \
    wait1;                                                                                         \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_SCK)

#define RECV_BIT(n, wait0, wait1)                                                                  \
    wait0;                                                                                         \
    ACC_PORT->BSRR = MASK_SET(PIN_ACC_SCK);                                                        \
    wait1;                                                                                         \
    b |= ((ACC_PORT->IDR >> (PIN_ACC_MISO & 0xf)) & 1) << n;                                       \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_SCK)

void bspi_send(const void *src, uint32_t len) {
    for (int i = 0; i < len; ++i) {
        uint8_t b = ((uint8_t *)src)[i];
        for (int j = 7; j >= 0; j--) {
            SEND_BIT(j, NOP0, NOP1);
        }
    }
    pin_set(PIN_ACC_MOSI, 0);
}

void bspi_recv(void *dst, uint32_t len) {
    for (int i = 0; i < len; ++i) {
        uint8_t b = 0x00;
        for (int j = 7; j >= 0; j--) {
            RECV_BIT(j, NOP0, NOP1);
        }
        ((uint8_t *)dst)[i] = b;
    }
}
