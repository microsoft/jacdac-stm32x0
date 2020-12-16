DEFINES += -DSTM32G031xx
OPENOCD ?= ./scripts/openocd -s ./scripts -f stlink-v2-1.cfg -f stm32g0x.cfg
include stm32g0.mk
