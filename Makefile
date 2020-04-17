PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
TARGET ?= jdm-v3

.SECONDARY: # this prevents object files from being removed

all: x-all

JD_CORE = jacdac-core

WARNFLAGS = -Wall -Wno-strict-aliasing
CFLAGS = $(DEFINES) \
	-mthumb -mfloat-abi=soft  \
	-Os -g3 -Wall -ffunction-sections -fdata-sections -nostartfiles \
	$(WARNFLAGS)
CONFIG_DEPS = \
	$(wildcard src/*.h) \
	$(wildcard bl/*.h) \
	$(wildcard $(PLATFORM)/*.h) \
	$(wildcard $(JD_CORE)/*.h) \
	$(wildcard targets/$(TARGET)/*.h) \
	targets/$(TARGET)/config.mk \
	Makefile

ifeq ($(BL),)
BUILT = built/$(TARGET)
else
BUILT = built/$(TARGET)/bl
endif

include targets/$(TARGET)/config.mk
BASE_TARGET ?= $(TARGET)

ifeq ($(BL),)
DEFINES += -DDEVICE_DMESG_BUFFER_SIZE=1024
C_SRC += $(wildcard src/*.c)
C_SRC += $(wildcard $(PLATFORM)/*.c)
C_SRC += $(JD_CORE)/jdlow.c
C_SRC += $(JD_CORE)/jdutil.c
C_SRC += $(HALSRC)
PROFILES = $(patsubst targets/$(TARGET)/profile/%.c,%,$(wildcard targets/$(TARGET)/profile/*.c))
else
DEFINES += -DDEVICE_DMESG_BUFFER_SIZE=0 -DBL
CPPFLAGS += -Ibl
C_SRC += $(wildcard bl/*.c)
C_SRC += $(PLATFORM)/pins.c
C_SRC += $(PLATFORM)/init.c
C_SRC += $(PLATFORM)/flash.c
C_SRC += src/dmesg.c
C_SRC += $(JD_CORE)/jdutil.c
AS_SRC += bl/boothandler.s
endif

ifneq ($(BMP),)
BMP_PORT = $(shell ls -1 /dev/cu.usbmodem????????1 | head -1)
endif


V = @

OBJ = $(addprefix $(BUILT)/,$(C_SRC:.c=.o) $(AS_SRC:.s=.o))

CPPFLAGS += \
	-Itargets/$(TARGET) \
	-Itargets/$(BASE_TARGET) \
	-I$(PLATFORM) \
	-Isrc \
	-I$(JD_CORE) \
	-I$(BUILT)

LDFLAGS = -specs=nosys.specs -specs=nano.specs \
	-T"$(LD_SCRIPT)" -Wl,-Map=$(BUILT)/output.map -Wl,--gc-sections

x-all: $(JD_CORE)/jdlow.c
	$(MAKE) -j8 build
ifeq ($(BL),)
	$(MAKE) -j8 BL=1 build
	$(V)$(PREFIX)size $(BUILT)/*.elf
endif

$(JD_CORE)/jdlow.c:
	if test -f ../pxt-common-packages/libs/jacdac/jdlow.c ; then \
		ln -s ../pxt-common-packages/libs/jacdac jacdac-core; \
	else \
		ln -s pxt-common-packages/libs/jacdac jacdac-core; \
	fi

r: run
l: flash-loop

run: all flash

drop:
	$(MAKE) TARGET=jdm-v3 all
	$(MAKE) TARGET=jdm-v3-bl all

ONCE ?= 1

flash: prep-built-gdb
ifeq ($(BMP),)
	$(OPENOCD) -c "program $(BUILT)/binary.elf verify reset exit"
else
ifeq ($(ONCE),)
	echo "set {int}0xe000ed0c = 0x5fa0004" >> built/debug.gdb
	echo "detach" >> built/debug.gdb
	echo "monitor swdp_scan" >> built/debug.gdb
	echo "attach 1" >> built/debug.gdb
	echo "bt" >> built/debug.gdb
endif
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
	echo "target extended-remote | $(OPENOCD) -f scripts/gdbdebug.cfg" >> built/debug.gdb
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

$(wildcard $(BUILT)/bl/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/src/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/$(PLATFORM)/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/$(JD_CORE)/*.o): $(CONFIG_DEPS)

$(BUILT)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo AS $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.bin: %.elf
	@echo BIN/HEX $<
	$(V)$(PREFIX)objcopy -O binary $< $@
	$(V)$(PREFIX)objcopy -O ihex $< $@

built/compress.js: scripts/compress.ts
	cd scripts; tsc

c: built/compress.js
	node $< tmp/images/*.bin

clean:
	rm -rf built

st:
	$(V)node scripts/map-file-stats.js  built/$(TARGET)/output.map

stf:
	$(V)node scripts/map-file-stats.js  built/$(TARGET)/output.map -fun

ifeq ($(BL),)
$(BUILT)/src/profile-%.o: targets/$(TARGET)/profile/%.c
	@echo CC $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILT)/binary-%.elf: $(BUILT)/src/profile-%.o $(OBJ) Makefile $(LD_SCRIPT) scripts/patch-bin.js
	@echo LD $@
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $< -lm
	@echo BL-PATCH $@
	$(V)node scripts/patch-bin.js $@ $(FLASH_SIZE) $(BL_SIZE)

build: $(addsuffix .bin,$(addprefix $(BUILT)/binary-,$(PROFILES)))
else
built/$(TARGET)/bl.elf: $(OBJ) Makefile $(LD_SCRIPT)
	@echo LD $@
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) -lm

build: built/$(TARGET)/bl.bin
endif

