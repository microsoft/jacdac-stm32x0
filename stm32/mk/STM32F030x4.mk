RAM_SIZE = 4
FLASH_SIZE = 16
BL_SIZE = 3
PAGE_SIZE = 1024
# STM32Cube requires x6 to be defined on x4 devices
DEFINES += -DSTM32F030x6
STARTUP_FILE = $(DRV)/CMSIS/Device/ST/STM32F0xx/Source/Templates/gcc/startup_stm32f030x6.s
