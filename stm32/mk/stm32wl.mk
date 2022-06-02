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

CPPFLAGS += 	\
	-I$(HALPATH)/Inc \
	-I$(PLATFORM)/STM32CubeWL/Drivers/CMSIS/Device/ST/STM32WLxx/Include \
	-I$(PLATFORM)/cmsis_core/Include

include $(PLATFORM)/mk/lora.mk
include $(PLATFORM)/mk/stm32.mk
