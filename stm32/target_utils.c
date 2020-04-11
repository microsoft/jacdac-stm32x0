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

static uint32_t murmur_hash2(uint32_t *d, int words) {
    const uint32_t m = 0x5bd1e995;
    uint32_t h = 0;

    while (words--) {
        uint32_t k = *d++;
        k *= m;
        k ^= k >> 24;
        k *= m;
        h *= m;
        h ^= k;
    }

    return h;
}

uint64_t device_id() {
    static uint64_t cache;
    if (!cache) {
        // serial: 0x330073 0x31365012 0x20313643
        // serial: 0x71007A 0x31365012 0x20313643
        // DMESG("serial: %x %x %x", ((uint32_t *)UID_BASE)[0], ((uint32_t *)UID_BASE)[1],
        //      ((uint32_t *)UID_BASE)[2]);

        // we hash everything - the entropy in the first word seems to be quite low
        // we also use two independant hashes
        uint8_t *uid = (uint8_t *)UID_BASE;
        uint32_t w1 = jd_hash_fnv1a(uid, 12);
        uint32_t w0 = murmur_hash2((uint32_t *)uid, 3);
        w0 &= ~0x02000000; // clear "universal" bit
        cache = (uint64_t)w0 << 32 | w1;
    }
    return cache;
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
