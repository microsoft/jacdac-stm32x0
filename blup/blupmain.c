#include "blup.h"

extern const unsigned char bootloader[];

int main(void) {
    __disable_irq();

    led_init();

    const uint32_t *sp = (const uint32_t *)bootloader;
    if (sp[0] != 0x2fd56055)
        JD_PANIC(); // bad magic
    uint32_t len = sp[1];
    sp += 4;

    uint8_t *flashend = (void *)(0x08000000 + JD_FLASH_SIZE);
    uint8_t *bladdr = (void *)(0x08000000 + JD_FLASH_SIZE - BL_SIZE);

    uint8_t *erase_ptr = bladdr;
    while (erase_ptr < flashend) {
        flash_erase(erase_ptr);
        erase_ptr += JD_FLASH_PAGE_SIZE;
    }

    len = (len + 7) & ~7;
    flash_program(bladdr, sp, len);

    // self-destruct
    uint32_t addr = (uint32_t)&app_handlers->app_reset_handler;
    addr &= ~7;
    uint64_t zero = 0;
    flash_program((void *)addr, &zero, sizeof(zero));

    target_reset();
}
