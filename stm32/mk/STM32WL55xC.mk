RAM_SIZE = 64
FLASH_SIZE ?= 256
BL_SIZE ?= 4
PAGE_SIZE = 2048
DEFINES += -DSTM32WL55xx
STARTUP_FILE = $(PLATFORM)/cmsis_device_wl/Source/Templates/gcc/startup_stm32wl55xx_cm4.s