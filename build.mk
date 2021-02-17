ifneq ($(TRG),)
TARGET := $(word 1,$(TRG))
PROF := $(word 2,$(TRG))
endif

ifeq ($(TARGET),)
$(error Define 'TRG = jm-v2.0 npx' or similar, best in Makefile.user)
endif

FORCE ?=

SCRIPTS = $(JD_STM)/scripts
PLATFORM = $(JD_STM)/stm32

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as

MAKE_FLAGS ?= -j8

WARNFLAGS = -Wall -Wno-strict-aliasing
CFLAGS = $(DEFINES) \
	-mthumb -mfloat-abi=soft  \
	-Os -g3 \
	-Wall -Wextra -Wno-unused-parameter -Wno-shift-negative-value -Wstrict-prototypes -Werror \
	-Wno-error=unused-function \
	-ffunction-sections -nostartfiles \
	$(WARNFLAGS) $(USERFLAGS)
CONFIG_DEPS = \
	$(wildcard $(JD_CORE)/inc/*.h) \
	$(wildcard $(JD_CORE)/inc/interfaces/*.h) \
	$(wildcard $(JD_CORE)/services/interfaces/*.h) \
	$(wildcard $(JD_STM)/src/*.h) \
	$(wildcard $(JD_STM)/bl/*.h) \
	$(wildcard $(PLATFORM)/*.h) \
	$(wildcard $(JD_CORE)/*.h) \
	$(wildcard targets/$(TARGET)/*.h) \
	targets/$(TARGET)/config.mk

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
C_SRC += $(wildcard $(JD_CORE)/source/*.c)
C_SRC += $(wildcard $(JD_CORE)/services/*.c)
C_SRC += $(wildcard $(JD_CORE)/drivers/*.c)
C_SRC += $(wildcard $(JD_CORE)/source/interfaces/simple_alloc.c)
C_SRC += $(wildcard $(JD_CORE)/source/interfaces/sensor.c)
C_SRC += $(wildcard $(JD_CORE)/source/interfaces/simple_rx.c)
C_SRC += $(wildcard $(JD_CORE)/source/interfaces/event_queue.c)
ifeq ($(BRIDGEQ),)
C_SRC += $(wildcard $(JD_CORE)/source/interfaces/tx_queue.c)
endif
C_SRC += $(wildcard $(JD_STM)/src/*.c)
C_SRC += $(wildcard $(PLATFORM)/*.c)
C_SRC += $(HALSRC)
else
DEFINES += -DDEVICE_DMESG_BUFFER_SIZE=0 -DBL
CPPFLAGS += -Ibl
C_SRC += $(wildcard $(JD_STM)/bl/*.c)
C_SRC += $(PLATFORM)/pins.c
C_SRC += $(PLATFORM)/init.c
C_SRC += $(PLATFORM)/flash.c
C_SRC += $(PLATFORM)/adc.c
C_SRC += $(JD_STM)/src/dmesg.c
C_SRC += $(JD_CORE)/source/jd_util.c
AS_SRC += $(JD_STM)/bl/boothandler.s
endif

ELF = $(BUILT_BIN)/$(PREF)-$(PROF).elf

ifneq ($(BMP),)
BMP_PORT ?= $(shell ls -1 /dev/cu.usbmodem????????1 | head -1)
endif


V = @

OBJ = $(addprefix $(BUILT)/,$(C_SRC:.c=.o) $(AS_SRC:.s=.o))
OBJ += $(BUILT_BIN)/version.o

CPPFLAGS += \
	-Itargets/$(TARGET) \
	-Itargets/$(BASE_TARGET) \
	-I$(PLATFORM) \
	-I$(JD_CORE)/inc \
	-I$(JD_STM)/src \
	-I$(JD_CORE) \
	-I$(BUILT)

LDFLAGS = -specs=nosys.specs -specs=nano.specs \
	-T"$(LD_SCRIPT)" -Wl,--gc-sections

all: refresh-version
	$(V)node $(SCRIPTS)/check-fw-id.js targets
	$(MAKE) $(MAKE_FLAGS) build
ifeq ($(BL),)
	$(MAKE) $(MAKE_FLAGS) BL=1 build
endif
ifeq ($(BL),)
	$(MAKE) combine
endif
	$(V)$(PREFIX)size $(BUILT_BIN)/*.elf

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
	while : ; do make ff && say done ; sleep 2 ; done

prep-built-gdb:
	echo "file $(ELF)" > built/debug.gdb
ifeq ($(BMP),)
	echo "target extended-remote | $(OPENOCD) -f $(SCRIPTS)/gdbdebug.cfg" >> built/debug.gdb
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

$(wildcard $(BUILT)/*/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/*/*/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/*/*/*/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/*/*/*/*/*.o): $(CONFIG_DEPS)
$(OBJ): $(CONFIG_DEPS)

$(BUILT)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo AS $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.hex: %.elf $(SCRIPTS)/bin2uf2.js
	@echo BIN/HEX $<
	$(V)$(PREFIX)objcopy -O binary $< $(@:.hex=.bin)
	$(V)$(PREFIX)objcopy -O ihex $< $@
ifeq ($(BL),)
	@echo UF2 $<
	$(V)node $(SCRIPTS)/bin2uf2.js $(@:.hex=.bin)
endif

built/compress.js: $(SCRIPTS)/compress.ts
	cd $(SCRIPTS); tsc

run-compress: built/compress.js
	node $< tmp/images/*.bin

clean:
	rm -rf built

st:
	$(V)node $(SCRIPTS)/map-file-stats.js $(ELF).map

stf:
	$(V)node $(SCRIPTS)/map-file-stats.js  $(ELF).map -fun

$(BUILT)/jd/prof-%.o: targets/$(TARGET)/profile/%.c
	@echo CC $<
	@mkdir -p $(BUILT)/jd
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

FW_VERSION = $(shell grep '"version":' package.json | sed -e 's/.*: "//; s/".*/-'"`date +%Y%m%d-%H%M`/")

refresh-version:
	@mkdir -p $(BUILT_BIN)
	echo 'const char app_fw_version[] = "$(FW_VERSION)";' > $(BUILT_BIN)/version.c

$(BUILT_BIN)/version.o: $(BUILT_BIN)/version.c
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<


$(BUILT_BIN)/$(PREF)-%.elf: $(BUILT)/jd/prof-%.o $(OBJ) Makefile $(LD_SCRIPT) $(SCRIPTS)/patch-bin.js $(FORCE)
	@echo LD $@
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$@.map  -o $@ $(OBJ) $< -lm
	@echo BIN-PATCH $@
	$(V)node $(SCRIPTS)/patch-bin.js -q $@ $(FLASH_SIZE) $(BL_SIZE) targets/$(TARGET)/profile

build: $(addsuffix .hex,$(addprefix $(BUILT_BIN)/$(PREF)-,$(PROFILES)))

combine: $(addsuffix .hex,$(addprefix $(BUILT_BIN)/combined-,$(PROFILES)))

$(BUILT_BIN)/combined-%.hex: $(BUILT_BIN)/$(PREF)-%.hex
	@echo COMBINE $@
	@(cat $< | grep -v '^:0.00000[51]' ; cat $(subst app-,bl-,$<)) > $@

# make sure we re-binary-patch the bootloader on every flash, to get different random seed
rc: run-combined
run-combined:
	touch $(SCRIPTS)/patch-bin.js
	$(MAKE) $(MAKE_FLAGS) BL=1 $(BUILT_BIN)/combined-$(PROF).hex
	$(MAKE) $(MAKE_FLAGS) $(BUILT_BIN)/combined-$(PROF).hex
	$(MAKE) ELF=$(BUILT_BIN)/combined-$(PROF).hex flash

force:
	@echo forcing...

DROP_TARGETS ?= jm-v2.0 jm-v2.0i jm-v2.0p jm-v2.1

targ-%:
	$(MAKE) TARGET=$(subst targ-,,$@)

drop: $(addprefix targ-,$(DROP_TARGETS))
	cd built; cat $(addsuffix /*.uf2,$(DROP_TARGETS)) > drop.uf2
	@ls -l built/drop.uf2

ff: full-flash

full-flash:
	$(MAKE) BL=1 r
	$(MAKE) r
