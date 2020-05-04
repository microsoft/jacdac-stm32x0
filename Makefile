TARGET ?= jd-v0
PROF ?= servo
FORCE ?=

.SECONDARY: # this prevents object files from being removed
.DEFAULT_GOAL := all

JD_CORE = jacdac-core

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as

WARNFLAGS = -Wall -Wno-strict-aliasing
CFLAGS = $(DEFINES) \
	-mthumb -mfloat-abi=soft  \
	-Os -g3 \
	-Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror \
	-Wno-error=unused-function \
	-ffunction-sections -fdata-sections -nostartfiles \
	$(WARNFLAGS)
CONFIG_DEPS = \
	$(wildcard jd/*.h) \
	$(wildcard lib/*.h) \
	$(wildcard bl/*.h) \
	$(wildcard $(PLATFORM)/*.h) \
	$(wildcard $(JD_CORE)/*.h) \
	$(wildcard targets/$(TARGET)/*.h) \
	targets/$(TARGET)/config.mk \
	Makefile

ifeq ($(BL),)
PREF = app
else
PREF = bl
endif
BUILT_BIN = built/$(TARGET)
BUILT = $(BUILT_BIN)/$(PREF)

include targets/$(TARGET)/config.mk
BASE_TARGET ?= $(TARGET)

PROFILES = $(patsubst targets/$(TARGET)/profile/%.c,%,$(wildcard targets/$(TARGET)/profile/*.c))

ifeq ($(BL),)
DEFINES += -DDEVICE_DMESG_BUFFER_SIZE=1024
C_SRC += $(wildcard jd/*.c)
C_SRC += $(wildcard lib/*.c)
C_SRC += $(wildcard $(PLATFORM)/*.c)
C_SRC += $(JD_CORE)/jdlow.c
C_SRC += $(JD_CORE)/jdutil.c
C_SRC += $(HALSRC)
else
DEFINES += -DDEVICE_DMESG_BUFFER_SIZE=0 -DBL
CPPFLAGS += -Ibl
C_SRC += $(wildcard bl/*.c)
C_SRC += $(PLATFORM)/pins.c
C_SRC += $(PLATFORM)/init.c
C_SRC += $(PLATFORM)/flash.c
C_SRC += lib/dmesg.c
C_SRC += $(JD_CORE)/jdutil.c
AS_SRC += bl/boothandler.s
endif

ELF = $(BUILT_BIN)/$(PREF)-$(PROF).elf

ifneq ($(BMP),)
BMP_PORT = $(shell ls -1 /dev/cu.usbmodem????????1 | head -1)
endif


V = @

OBJ = $(addprefix $(BUILT)/,$(C_SRC:.c=.o) $(AS_SRC:.s=.o))

CPPFLAGS += \
	-Itargets/$(TARGET) \
	-Itargets/$(BASE_TARGET) \
	-I$(PLATFORM) \
	-Ijd \
	-Ilib \
	-I$(JD_CORE) \
	-I$(BUILT)

LDFLAGS = -specs=nosys.specs -specs=nano.specs \
	-T"$(LD_SCRIPT)" -Wl,--gc-sections

all: $(JD_CORE)/jdlow.c
	$(MAKE) -j8 build
ifeq ($(BL),)
	$(MAKE) -j8 BL=1 build
endif
ifeq ($(BL),)
	$(MAKE) combine
endif
	$(V)$(PREFIX)size $(BUILT_BIN)/*.elf

$(JD_CORE)/jdlow.c:
	if test -f ../pxt-common-packages/libs/jacdac/jdlow.c ; then \
		ln -s ../pxt-common-packages/libs/jacdac jacdac-core; \
	else \
		ln -s pxt-common-packages/libs/jacdac jacdac-core; \
	fi

r: run
l: flash-loop

run: all flash

ONCE ?= 1

flash: prep-built-gdb
ifeq ($(BMP),)
	$(OPENOCD) -c "program $(ELF) verify reset exit"
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
	echo "file $(ELF)" > built/debug.gdb
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
$(wildcard $(BUILT)/jd/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/lib/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/$(PLATFORM)/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/$(JD_CORE)/*.o): $(CONFIG_DEPS)

$(BUILT)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo AS $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.hex: %.elf scripts/bin2uf2.js
	@echo BIN/HEX $<
	$(V)$(PREFIX)objcopy -O binary $< $(@:.hex=.bin)
	$(V)$(PREFIX)objcopy -O ihex $< $@
ifeq ($(BL),)
	@echo UF2 $<
	$(V)node scripts/bin2uf2.js $(@:.hex=.bin)
endif

built/compress.js: scripts/compress.ts
	cd scripts; tsc

run-compress: built/compress.js
	node $< tmp/images/*.bin

clean:
	rm -rf built

st:
	$(V)node scripts/map-file-stats.js $(BUILT)/output.map

stf:
	$(V)node scripts/map-file-stats.js  $(BUILT)/output.map -fun

$(BUILT)/jd/prof-%.o: targets/$(TARGET)/profile/%.c
	@echo CC $<
	@mkdir -p $(BUILT)/jd
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILT_BIN)/$(PREF)-%.elf: $(BUILT)/jd/prof-%.o $(OBJ) Makefile $(LD_SCRIPT) scripts/patch-bin.js $(FORCE)
	@echo LD $@
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$@.map  -o $@ $(OBJ) $< -lm
	@echo BIN-PATCH $@
	$(V)node scripts/patch-bin.js -q $@ $(FLASH_SIZE) $(BL_SIZE) targets/$(TARGET)/profile	

build: $(addsuffix .hex,$(addprefix $(BUILT_BIN)/$(PREF)-,$(PROFILES)))

combine: $(addsuffix .hex,$(addprefix $(BUILT_BIN)/combined-,$(PROFILES)))

$(BUILT_BIN)/combined-%.hex: $(BUILT_BIN)/$(PREF)-%.hex
	@echo COMBINE $@
	@(cat $< | grep -v '^:0.00000[51]' ; cat $(subst app-,bl-,$<)) > $@

# make sure we re-binary-patch the bootloader on every flash, to get different random seed
rc: run-combined
run-combined:
	touch scripts/patch-bin.js
	$(MAKE) -j8 BL=1 $(BUILT_BIN)/combined-$(PROF).hex
	$(MAKE) -j8 $(BUILT_BIN)/combined-$(PROF).hex
	$(MAKE) ELF=$(BUILT_BIN)/combined-$(PROF).hex flash

force:
	@echo forcing...
