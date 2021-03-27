#include "jdstm.h"

#ifdef PIN_ACC_SCK

#define MASK_SET(p) (1 << ((p)&0xf))
#define MASK_CLR(p) (1 << (((p)&0xf) + 16))

#ifdef STM32F0
#define LP_NOP0 ((void)0)
#define LP_NOP1 asm volatile("nop \n nop \n nop")
#define PLL_NOP0 target_wait_cycles(1)
#define PLL_NOP1 target_wait_cycles(7)
#elif defined(STM32G0)
#ifdef FAST_BSPI
// this is 1-1.5MHz
#define LP_NOP0 ((void)0)
#define LP_NOP1 asm volatile("nop \n nop \n nop")
#else
// this is about 0.5MHz
#define LP_NOP0 target_wait_cycles(1)
#define LP_NOP1 target_wait_cycles(1)
#endif
#define PLL_NOP0 target_wait_cycles(3)
#define PLL_NOP1 target_wait_cycles(10)
#else
#error "todo"
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

// someone might enabled PLL in the middle of this function, so go safe with PLL delays

void bspi_send(const void *src, uint32_t len) {
    for (unsigned i = 0; i < len; ++i) {
        uint8_t b = ((uint8_t *)src)[i];
        if (pwr_in_pll())
            for (int j = 7; j >= 0; j--) {
                SEND_BIT(j, PLL_NOP0, PLL_NOP1);
            }
        else
            for (int j = 7; j >= 0; j--) {
                SEND_BIT(j, LP_NOP0, LP_NOP1);
            }
    }
    pin_set(PIN_ACC_MOSI, 0);
}

void bspi_recv(void *dst, uint32_t len) {
    for (unsigned i = 0; i < len; ++i) {
        uint8_t b = 0x00;
        if (pwr_in_pll())
            for (int j = 7; j >= 0; j--) {
                RECV_BIT(j, PLL_NOP0, PLL_NOP1);
            }
        else
            for (int j = 7; j >= 0; j--) {
                RECV_BIT(j, LP_NOP0, LP_NOP1);
            }
        ((uint8_t *)dst)[i] = b;
    }
}

void bspi_init() {
    pin_setup_output(PIN_ACC_MOSI);
    pin_setup_output(PIN_ACC_SCK);
    pin_setup_input(PIN_ACC_MISO, -1);
    pin_setup_output(PIN_ACC_CS);
    pin_set(PIN_ACC_CS, 1);
}

#endif
