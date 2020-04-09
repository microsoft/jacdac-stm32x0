DEFINES += -DSTM32G031xx
include stm32g0.mk
OPENOCD = ./scripts/openocd -s ./scripts -f stlink-v2-1.cfg -f stm32g0x.cfg
