SERIES = F0
CFLAGS += -mcpu=cortex-m0

#OPENOCD = openocd -f interface/stlink-v2-1.cfg -f target/stm32f0x.cfg
OPENOCD = ./scripts/openocd -s ./scripts -f cmsis-dap.cfg -f stm32f0x.cfg

HALPREF = $(DRV)/STM32F0xx_HAL_Driver/Src
HALSRC =  \
$(HALPREF)/stm32f0xx_ll_adc.c \
$(HALPREF)/stm32f0xx_ll_comp.c \
$(HALPREF)/stm32f0xx_ll_crc.c \
$(HALPREF)/stm32f0xx_ll_crs.c \
$(HALPREF)/stm32f0xx_ll_dac.c \
$(HALPREF)/stm32f0xx_ll_dma.c \
$(HALPREF)/stm32f0xx_ll_exti.c \
$(HALPREF)/stm32f0xx_ll_gpio.c \
$(HALPREF)/stm32f0xx_ll_i2c.c \
$(HALPREF)/stm32f0xx_ll_pwr.c \
$(HALPREF)/stm32f0xx_ll_rcc.c \
$(HALPREF)/stm32f0xx_ll_rtc.c \
$(HALPREF)/stm32f0xx_ll_spi.c \
$(HALPREF)/stm32f0xx_ll_tim.c \
$(HALPREF)/stm32f0xx_ll_usart.c \
$(HALPREF)/stm32f0xx_ll_utils.c \

BMP = 1

PLATFORM = stm32

# TODO move some of this to common stm32.mk
DEFINES += -D$(MCU)
AS_SRC = $(DRV)/CMSIS/Device/ST/STM32F0xx/Source/Templates/gcc/startup_$(MCU:STM32F%=stm32f%).s
LD_SCRIPT = ld/$(MCU).ld
CUBE = stm32/STM32Cube$(SERIES)
DRV = $(CUBE)/Drivers
CPPFLAGS += 	\
	-I$(DRV)/STM32$(SERIES)xx_HAL_Driver/Inc \
	-I$(DRV)/STM32$(SERIES)xx_HAL_Driver/Inc/Legacy \
	-I$(DRV)/CMSIS/Device/ST/STM32$(SERIES)xx/Include \
	-I$(DRV)/CMSIS/Include
DEFINES += -DUSE_FULL_ASSERT -DUSE_FULL_LL_DRIVER -DSTM32$(SERIES)
