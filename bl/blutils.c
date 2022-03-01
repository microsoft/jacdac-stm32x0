#include "blutil.h"

RAM_FUNC
void target_wait_cycles(int n) {
    __asm__ __volatile__(".syntax unified\n"
                         "1:              \n"
                         "   subs %0, #1   \n" // subtract 1 from %0 (n)
#if defined(STM32G0) || defined(STM32WL)
                         "  nop  \n"
#endif
                         "   bne 1b       \n" // if result is not 0 jump to 1
                         : "+r"(n)            // '%0' is n variable with RW constraints
                         :                    // no input
                         :                    // no clobber
    );
}

void target_wait_us(uint32_t n) {
#if defined(STM32G0) || defined(STM32F0) || defined(STM32WL)
    n = n * CPU_MHZ >> 2;
#else
#error "define clock rate"
#endif
    target_wait_cycles(n);
}

void target_reset(void) {
    NVIC_SystemReset();
}

void __libc_init_array(void) {
    // do nothing - not using static constructors
}

void target_enable_irq(void) {}

void target_disable_irq(void) {}
