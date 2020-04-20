#include "jdstm.h"

#define WAIT_BUSY()                                                                                \
    while (FLASH->SR & FLASH_SR_BSY)                                                               \
        ;

static void unlock(void) {
    WAIT_BUSY();
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

static void lock(void) {
    FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER);
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
    if ((((uint32_t)dst) & 1) || (((uint32_t)src) & 1) || (len & 1))
        jd_panic();

    unlock();
    FLASH->CR |= FLASH_CR_PG; // enable programming

    len >>= 1;

    while (len--) {
        *(__IO uint16_t *)(dst) = *(uint16_t *)src;
        check_eop();
        dst = (uint16_t *)dst + 1;
        src = (uint16_t *)src + 1;
    }

    lock();
}

void flash_erase(void *page_addr) {
    unlock();
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = (uint32_t)page_addr;
    FLASH->CR |= FLASH_CR_STRT;
    check_eop();
    lock();
}