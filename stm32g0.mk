SERIES = G0
CFLAGS += -mcpu=cortex-m0plus

OPENOCD = ./scripts/openocd -s ./scripts -f cmsis-dap.cfg -f stm32g0x.cfg

HALPREF = $(DRV)/STM32G0xx_HAL_Driver/Src
HALSRC =  \
$(HALPREF)/stm32g0xx_ll_adc.c \
$(HALPREF)/stm32g0xx_ll_comp.c \
$(HALPREF)/stm32g0xx_ll_crc.c \
$(HALPREF)/stm32g0xx_ll_dac.c \
$(HALPREF)/stm32g0xx_ll_dma.c \
$(HALPREF)/stm32g0xx_ll_exti.c \
$(HALPREF)/stm32g0xx_ll_gpio.c \
$(HALPREF)/stm32g0xx_ll_i2c.c \
$(HALPREF)/stm32g0xx_ll_lptim.c \
$(HALPREF)/stm32g0xx_ll_lpuart.c \
$(HALPREF)/stm32g0xx_ll_pwr.c \
$(HALPREF)/stm32g0xx_ll_rcc.c \
$(HALPREF)/stm32g0xx_ll_rng.c \
$(HALPREF)/stm32g0xx_ll_rtc.c \
$(HALPREF)/stm32g0xx_ll_spi.c \
$(HALPREF)/stm32g0xx_ll_tim.c \
$(HALPREF)/stm32g0xx_ll_ucpd.c \
$(HALPREF)/stm32g0xx_ll_usart.c \
$(HALPREF)/stm32g0xx_ll_utils.c \
