PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as

TARGET ?= jdm-v3

JD_CORE = jacdac-core

CUBE = STM32Cube$(SERIES)
DRV = $(CUBE)/Drivers
DEFINES = -DUSE_FULL_ASSERT -DUSE_FULL_LL_DRIVER -DSTM32$(SERIES) -DDEVICE_DMESG_BUFFER_SIZE=1024
WARNFLAGS = -Wall -Wno-strict-aliasing
CFLAGS = $(DEFINES) \
	-mthumb -mfloat-abi=soft  \
	-Os -g3 -Wall -ffunction-sections -fdata-sections \
	$(WARNFLAGS)
BUILT = built/$(TARGET)
HEADERS = $(wildcard src/*.h) $(wildcard $(JD_CORE)/*.h) $(wildcard targets/$(TARGET)/*.h)

include targets/$(TARGET)/config.mk
BASE_TARGET ?= $(TARGET)

ifneq ($(BMP),)
BMP_PORT = $(shell ls -1 /dev/cu.usbmodem????????1 | head -1)
endif

C_SRC += $(wildcard src/*.c) 
C_SRC += $(JD_CORE)/jdlow.c
C_SRC += $(JD_CORE)/jdutil.c
C_SRC += $(HALSRC)

AS_SRC = targets/$(BASE_TARGET)/startup.s
LD_SCRIPT = targets/$(BASE_TARGET)/linker.ld

V = @

OBJ = $(addprefix $(BUILT)/,$(C_SRC:.c=.o) $(AS_SRC:.s=.o))

CPPFLAGS = \
	-I$(DRV)/STM32$(SERIES)xx_HAL_Driver/Inc \
	-I$(DRV)/STM32$(SERIES)xx_HAL_Driver/Inc/Legacy \
	-I$(DRV)/CMSIS/Device/ST/STM32$(SERIES)xx/Include \
	-I$(DRV)/CMSIS/Include \
	-Itargets/$(TARGET) \
	-Itargets/$(BASE_TARGET) \
	-Isrc \
	-I$(JD_CORE) \
	-I$(BUILT)

LDFLAGS = -specs=nosys.specs -specs=nano.specs \
	-T"$(LD_SCRIPT)" -Wl,-Map=$(BUILT)/output.map -Wl,--gc-sections

all:
	$(MAKE) -j8 $(BUILT)/binary.hex

drop:
	$(MAKE) TARGET=g031 all
	$(MAKE) TARGET=f031 all

r: run
l: flash-loop

run: all flash

flash: prep-built-gdb
ifeq ($(BMP),)
	$(OPENOCD) -c "program $(BUILT)/binary.elf verify reset exit"
else
	echo "load" >> built/debug.gdb
	echo "quit" >> built/debug.gdb
	arm-none-eabi-gdb --command=built/debug.gdb < /dev/null 2>&1 | tee built/flash.log
	grep -q "Start address" built/flash.log
endif

flash-loop: all
	while : ; do make flash && break ; sleep 1 ; done

prep-built-gdb:
	echo "file $(BUILT)/binary.elf" > built/debug.gdb
ifeq ($(BMP),)
	echo "target extended-remote | $(OPENOCD) -f gdbdebug.cfg" >> built/debug.gdb
else
	echo "target extended-remote $(BMP_PORT)" >> built/debug.gdb
	echo "monitor swdp_scan" >> built/debug.gdb
	echo "attach 1" >> built/debug.gdb
endif

gdb: prep-built-gdb
	arm-none-eabi-gdb --command=built/debug.gdb

$(BUILT)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(wildcard $(BUILT)/src/*.o): $(HEADERS)
$(wildcard $(BUILT)/$(JD_CORE)/*.o): $(HEADERS)

$(BUILT)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo AS $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILT)/binary.elf: $(OBJ) Makefile
	@echo LD $@
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) -lm

$(BUILT)/binary.hex: $(BUILT)/binary.elf
	@echo HEX $<
	$(PREFIX)objcopy -O ihex $< $@
	$(PREFIX)size $<

$(BUILT)/addata.h: $(BUILT)/genad
	./$(BUILT)/genad > "$@"

$(BUILT)/genad: genad/genad.c $(HEADERS)
	cc -Isrc -o "$@" $<

clean:
	rm -rf built
