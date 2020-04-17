#include "jdstm.h"

RAM_FUNC
void target_wait_cycles(int n) {
    __asm__ __volatile__(".syntax unified\n"
                         "1:              \n"
                         "   subs %0, #1   \n" // subtract 1 from %0 (n)
                         "   bne 1b       \n"  // if result is not 0 jump to 1
                         : "+r"(n)             // '%0' is n variable with RW constraints
                         :                     // no input
                         :                     // no clobber
    );
}

void target_wait_us(uint32_t n) {
#ifdef STM32G0
    n = n * cpu_mhz / 3;
#elif defined(STM32F0)
    n = n * cpu_mhz / 4;
#else
#error "define clock rate"
#endif
    target_wait_cycles(n);
}

void target_reset() {
    NVIC_SystemReset();
}

static int8_t irq_disabled;

void target_enable_irq() {
    irq_disabled--;
    if (irq_disabled <= 0) {
        irq_disabled = 0;
        asm volatile("cpsie i" : : : "memory");
    }
}

void target_disable_irq() {
    asm volatile("cpsid i" : : : "memory");
    irq_disabled++;
}

int target_in_irq() {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void __libc_init_array() {
    // do nothing - not using static constructors
}
