NOBL = 1 # temp

SERIES = WL
CFLAGS += -mcpu=cortex-m4

#OPENOCD ?= openocd -f interface/stlink-v2-1.cfg -f target/stm32wlx.cfg
#OPENOCD ?= ./scripts/openocd -s ./scripts -f cmsis-dap.cfg -f stm32wlx.cfg
OPENOCD ?= openocd -f interface/cmsis-dap.cfg -f target/stm32wlx.cfg #???

HALPATH = $(PLATFORM)/STM32CubeWL/Drivers/STM32WLxx_HAL_Driver
HALPREF = $(HALPATH)/Src
HALSRC =  \
$(HALPREF)/stm32wlxx_ll_adc.c \
$(HALPREF)/stm32wlxx_ll_comp.c \
$(HALPREF)/stm32wlxx_ll_dac.c \
$(HALPREF)/stm32wlxx_ll_dma.c \
$(HALPREF)/stm32wlxx_ll_exti.c \
$(HALPREF)/stm32wlxx_ll_gpio.c \
$(HALPREF)/stm32wlxx_ll_i2c.c \
$(HALPREF)/stm32wlxx_ll_pwr.c \
$(HALPREF)/stm32wlxx_ll_rcc.c \
$(HALPREF)/stm32wlxx_ll_rtc.c \
$(HALPREF)/stm32wlxx_ll_spi.c \
$(HALPREF)/stm32wlxx_ll_tim.c \
$(HALPREF)/stm32wlxx_ll_lptim.c \
$(HALPREF)/stm32wlxx_ll_usart.c \
$(HALPREF)/stm32wlxx_ll_utils.c \

BMP ?= 1

LD_FLASH_SIZE ?= $(FLASH_SIZE)

# TODO move some of this to common stm32.mk
AS_SRC = $(STARTUP_FILE)
CPPFLAGS += 	\
	-I$(HALPATH)/Inc \
	-I$(PLATFORM)/STM32CubeWL/Drivers/CMSIS/Device/ST/STM32WLxx/Include \
	-I$(PLATFORM)/cmsis_core/Include
DEFINES += -DUSE_FULL_LL_DRIVER -DSTM32$(SERIES)
DEFINES += -D$(MCU) -DJD_FLASH_SIZE="1024*$(FLASH_SIZE)" -DBL_SIZE="1024*$(BL_SIZE)"

include $(PLATFORM)/mk/lora.mk

include $(PLATFORM)/mk/$(MCU).mk

CONFIG_DEPS += $(wildcard $(PLATFORM)/mk/*.mk)

LD_SCRIPT = $(BUILT)/linker.ld
$(BUILT)/linker.ld: $(wildcard $(PLATFORM)/mk/*.mk) Makefile
	mkdir -p $(BUILT)
	: > $@
	echo "MEMORY {" >> $@
	echo "RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = $(RAM_SIZE)K" >> $@
ifeq ($(BL),)
# The -12 bytes is required by the flashing process, at least with BMP
	echo "FLASH (rx)  : ORIGIN = 0x8000000, LENGTH = $(LD_FLASH_SIZE)K - $(BL_SIZE)K - 12" >> $@
	echo "}" >> $@
	echo "INCLUDE $(JD_STM)/ld/gcc_arm.ld" >> $@
else
	echo "FLASH (rx)  : ORIGIN = 0x8000000 + $(LD_FLASH_SIZE)K - $(BL_SIZE)K, LENGTH = $(BL_SIZE)K" >> $@
	echo "}" >> $@
	echo "INCLUDE $(JD_STM)/ld/gcc_arm_bl_at_end.ld" >> $@
endif
