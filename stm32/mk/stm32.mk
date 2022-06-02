BMP ?= 1

LD_FLASH_SIZE ?= $(FLASH_SIZE)
FLASH_SETTINGS_SIZE ?= 0

AS_SRC = $(STARTUP_FILE)
DEFINES += -DUSE_FULL_LL_DRIVER -DSTM32$(SERIES)
DEFINES += -D$(MCU) -DJD_FLASH_SIZE="1024*$(FLASH_SIZE)" -DJD_FLASH_PAGE_SIZE=$(PAGE_SIZE) -DBL_SIZE="1024*$(BL_SIZE)" -DJD_FLASH_SETTINGS_SIZE="1024*$(FLASH_SETTINGS_SIZE)" 

include $(PLATFORM)/mk/$(MCU).mk

CONFIG_DEPS += $(wildcard $(PLATFORM)/mk/*.mk)

LD_SCRIPT = $(BUILT)/linker.ld
$(BUILT)/linker.ld: $(wildcard $(PLATFORM)/mk/*.mk) Makefile targets/$(TARGET)/config.mk
	mkdir -p $(BUILT)
	: > $@
	echo "MEMORY {" >> $@
	echo "RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = $(RAM_SIZE)K" >> $@
ifeq ($(BL),)
# The -12 bytes is required by the flashing process, at least with BMP
	echo "FLASH (rx)  : ORIGIN = 0x8000000, LENGTH = $(LD_FLASH_SIZE)K - $(BL_SIZE)K - $(FLASH_SETTINGS_SIZE)K - 12" >> $@
	echo "}" >> $@
	echo "INCLUDE $(JD_STM)/ld/gcc_arm.ld" >> $@
else
	echo "FLASH (rx)  : ORIGIN = 0x8000000 + $(LD_FLASH_SIZE)K - $(BL_SIZE)K, LENGTH = $(BL_SIZE)K" >> $@
	echo "}" >> $@
	echo "INCLUDE $(JD_STM)/ld/gcc_arm_bl_at_end.ld" >> $@
endif
