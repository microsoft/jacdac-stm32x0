RAM_SIZE = 6
FLASH_SIZE ?= 32
BL_SIZE ?= 3
PAGE_SIZE = 1024
DEFINES += -DSTM32F042x6
STARTUP_FILE = $(PLATFORM)/cmsis_device_f0/Source/Templates/gcc/startup_stm32f042x6.s