#include "jdstm.h"

#ifdef STM32G0
#define FLASH_SR_BSY FLASH_SR_BSY1
#endif

#define WAIT_BUSY()                                                                                \
    while (FLASH->SR & FLASH_SR_BSY)                                                               \
        ;

static void unlock(void) {
    WAIT_BUSY();
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
    FLASH->SR = FLASH_SR_EOP;
    FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_STRT);
#ifdef STM32G0
    // this is required for the EOP/error bit to be set
    FLASH->CR |= FLASH_CR_EOPIE | FLASH_CR_ERRIE;
#endif
}

static void lock(void) {
    // FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER);
    FLASH->CR |= FLASH_CR_LOCK;
}

static void check_eop(void) {
    WAIT_BUSY();
    if (FLASH->SR & FLASH_SR_EOP)
        FLASH->SR = FLASH_SR_EOP;
    else
        jd_panic();
}

void flash_program(void *dst, const void *src, uint32_t len) {
#ifdef STM32G0
    if ((((uint32_t)dst) & 7) || (((uint32_t)src) & 7) || (len & 7))
        jd_panic();
#else
    if ((((uint32_t)dst) & 1) || (((uint32_t)src) & 1) || (len & 1))
        jd_panic();
#endif

    unlock();
    FLASH->CR |= FLASH_CR_PG; // enable programming

#ifdef STM32G0
    len >>= 3;
    __IO uint32_t *dp = dst;
    const uint32_t *sp = src;
    while (len--) {
        int erased = dp[0] + 1 == 0 && dp[1] + 1 == 0;
        if (!erased && (sp[0] || sp[1]))
            jd_panic();

        WAIT_BUSY();
        dp[0] = sp[0];
        dp[1] = sp[1];
        check_eop();
        dp += 2;
        sp += 2;
    }
#else
    len >>= 1;
    while (len--) {
        *(__IO uint16_t *)(dst) = *(uint16_t *)src;
        check_eop();
        dst = (uint16_t *)dst + 1;
        src = (uint16_t *)src + 1;
    }
#endif

    lock();
}

void flash_erase(void *page_addr) {
    unlock();
#ifdef STM32G0
    uint32_t addrmask = (((uint32_t)page_addr >> 11) << FLASH_CR_PNB_Pos) & FLASH_CR_PNB;
    FLASH->CR = (FLASH->CR & ~FLASH_CR_PNB) | FLASH_CR_PER | addrmask;
#else
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = (uint32_t)page_addr;
#endif
    FLASH->CR |= FLASH_CR_STRT;
    check_eop();
    lock();
}