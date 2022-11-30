#include "jdstm.h"

#if defined(STM32G0) || defined(STM32WL)
#define NEW_FLASH 1
#else
#define NEW_FLASH 0
#endif

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
    FLASH->SR = FLASH_SR_EOP; // write to clear
    FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_STRT);
#if NEW_FLASH
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
    // EOP should be set when not busy anymore
    if (FLASH->SR & FLASH_SR_EOP)
        FLASH->SR = FLASH_SR_EOP; // OK, write to clear
    else
        JD_PANIC(); // otherwise, whoops!
}

void flash_program(void *dst, const void *src, uint32_t len) {
#if NEW_FLASH
    // G0/WL flash requires 64 bit writes
    if ((((uint32_t)dst) & 7) || (((uint32_t)src) & 3) || (len & 7))
        JD_PANIC();
#else
    // F0 flash can handle 16 bit writes
    if ((((uint32_t)dst) & 1) || (((uint32_t)src) & 1) || (len & 1))
        JD_PANIC();
#endif

    unlock();
    FLASH->CR |= FLASH_CR_PG; // enable programming

#if NEW_FLASH
    len >>= 3;
    __IO uint32_t *dp = dst;
    const uint32_t *sp = src;
    while (len--) {
        // check if dp[0/1] == 0xffffffff
        int erased = dp[0] + 1 == 0 && dp[1] + 1 == 0;
        if (!erased && (sp[0] || sp[1]))
            JD_PANIC();

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
#if NEW_FLASH
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

void flash_sync() {
    // do nothing
}
