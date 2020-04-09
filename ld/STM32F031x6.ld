MEMORY
{
    FLASH (rx)  : ORIGIN = 0x8000000, LENGTH = 32K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 4K
}
INCLUDE ld/gcc_arm.ld
