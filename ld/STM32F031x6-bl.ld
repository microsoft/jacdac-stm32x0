MEMORY
{
    /* Last 4k is for bootloader */
    FLASH (rx)  : ORIGIN = 0x8000000 + 32K - 4K, LENGTH = 4K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 4K
}
INCLUDE ld/gcc_arm_bl_at_end.ld
