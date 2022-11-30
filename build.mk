ifneq ($(TRG),)
TRG0 := $(subst /profile/, ,$(TRG))
TARGET := $(subst targets/,,$(word 1,$(TRG0)))
PROF := $(word 2,$(TRG0:.c=))
endif

ifeq ($(TARGET),)
$(error Define 'TRG = targets/buzzer-v1.0/profile/buzzer.c' or similar, best in Makefile.user)
endif

FORCE ?=

SCRIPTS = $(JD_STM)/scripts
PLATFORM = $(JD_STM)/stm32

PREFIX = arm-none-eabi-
CC = $(CCACHE) $(PREFIX)gcc
AS = $(CCACHE) $(PREFIX)as

MAKE_FLAGS ?= -j8

WARNFLAGS = -Wall -Wno-strict-aliasing
CFLAGS = $(DEFINES) \
	-mthumb -mfloat-abi=soft  \
	-Os -g3 -DSTM32X=1 \
	-Wall -Wextra -Wno-unused-parameter -Wno-shift-negative-value -Wstrict-prototypes \
	-Wno-zero-length-bounds \
	-ffunction-sections -nostartfiles \
	$(WARNFLAGS) $(USERFLAGS)
CONFIG_DEPS = \
	$(wildcard $(JD_CORE)/inc/*.h) \
	$(wildcard $(JD_CORE)/inc/interfaces/*.h) \
	$(wildcard $(JD_CORE)/inc/click/*.h) \
	$(wildcard $(JD_CORE)/services/*.h) \
	$(wildcard $(JD_CORE)/services/interfaces/*.h) \
	$(wildcard $(JD_STM)/src/*.h) \
	$(wildcard $(JD_STM)/bl/*.h) \
	$(wildcard $(PLATFORM)/*.h) \
	$(wildcard $(JD_CORE)/*.h) \
	$(wildcard addons/*.h) \
	$(wildcard targets/$(TARGET)/*.h) \
	targets/$(TARGET)/config.mk

ifneq ($(BL),)
PREF = bl
override APP =
override BLUP =
endif

ifneq ($(BLUP),)
PREF = blup
override APP =
override BL =
endif

ifeq ($(BL)$(BLUP),)
PREF = app
override APP = 1
override BL =
override BLUP =
endif

BUILT_BIN = built/$(TARGET)
BUILT = $(BUILT_BIN)/$(PREF)

ifeq ($(DROP_TARGETS),acme-corp-button)
_IGNORE_DROP := $(shell test -d targets/acme-corp-button || cp -r targets/_example targets/acme-corp-button)
endif

include targets/$(TARGET)/config.mk
BASE_TARGET ?= $(TARGET)

PROFILES = $(patsubst targets/$(TARGET)/profile/%.c,%,$(wildcard targets/$(TARGET)/profile/*.c))

ifneq ($(APP),)
C_SRC += $(wildcard $(JD_CORE)/source/*.c)
C_SRC += $(wildcard $(JD_CORE)/services/*.c)
C_SRC += $(wildcard $(JD_CORE)/drivers/*.c)
C_SRC += $(wildcard $(JD_CORE)/click/*.c)
C_SRC += $(wildcard addons/*.c)
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
endif

ifneq ($(BL)$(BLUP),)
C_SRC += $(PLATFORM)/pins.c
C_SRC += $(PLATFORM)/init.c
C_SRC += $(PLATFORM)/flash.c
C_SRC += $(PLATFORM)/adc.c
C_SRC += $(JD_STM)/src/dmesg.c
C_SRC += $(JD_CORE)/source/jd_util.c
endif

ifneq ($(BL),)
DEFINES += -DDEVICE_DMESG_BUFFER_SIZE=0 -DBL
C_SRC += $(wildcard $(JD_STM)/bl/*.c)
AS_SRC += $(JD_STM)/bl/boothandler.s
endif

ifneq ($(BLUP),)
DEFINES += -DBLUP
CPPFLAGS += -I$(JD_STM)/bl
C_SRC += $(JD_STM)/src/utils.c
C_SRC += $(JD_STM)/bl/blutils.c
C_SRC += $(JD_STM)/bl/blpwm.c
C_SRC += $(JD_STM)/bl/blled.c
C_SRC += $(wildcard $(JD_STM)/blup/*.c)
endif

ifeq ($(APP)$(JD_CLIENT),11)
DEFINES += -DJD_CLIENT=1
C_SRC += $(wildcard $(JD_CORE)/client/*.c)
C_SRC += $(wildcard $(JD_CORE)/devicescript/*.c)
C_SRC += $(wildcard $(JD_CORE)/storage/*.c)
C_SRC += $(wildcard $(JD_CORE)/storage/ff/*.c)
C_SRC += $(JD_CORE)/jacdac/dist/c/jd_spec_pack.c
CPPFLAGS += -I$(JD_CORE)/devicescript
CONFIG_DEPS += \
	$(wildcard $(JD_CORE)/client/*.h) \
	$(wildcard $(JD_CORE)/devicescript/*.h)
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
	-I$(JD_CORE)/inc/click \
	-I$(JD_CORE)/services \
	-I$(JD_STM)/src \
	-I$(JD_CORE) \
	-I$(BUILT) \
	-I.

LDFLAGS = -specs=nosys.specs -specs=nano.specs \
	-T"$(LD_SCRIPT)" -Wl,--gc-sections

all: refresh-version
ifeq ($(BL)$(NOBL),)
	$(V)node $(SCRIPTS)/check-fw-id.js targets
	$(MAKE) $(MAKE_FLAGS) BL=1 build
endif
	$(MAKE) $(MAKE_FLAGS) build
ifeq ($(BL)$(NOBL),)
	$(MAKE) combine
endif
	$(V)$(PREFIX)size $(BUILT_BIN)/*.elf
	@echo
	@echo "*** Build OK"
	@echo

r: run
l: flash-loop

run: all flash

ONCE ?= 1

flash: prep-built-gdb
ifeq ($(BMP),)
	$(OPENOCD) -c "program $(ELF) verify reset exit"
else
ifneq ($(HARD_ERASE),)
	#echo "monitor option erase" >> built/debug.gdb
	echo "monitor option write 0x3FFFF0AA" >> built/debug.gdb
	echo "detach" >> built/debug.gdb
	echo "monitor swdp_scan" >> built/debug.gdb
	echo "attach 1" >> built/debug.gdb
endif
ifeq ($(ONCE),)
	echo "set {int}0xe000ed0c = 0x5fa0004" >> built/debug.gdb
	echo "detach" >> built/debug.gdb
	echo "monitor swdp_scan" >> built/debug.gdb
	echo "attach 1" >> built/debug.gdb
endif
	echo "load" >> built/debug.gdb
	echo "quit" >> built/debug.gdb
	arm-none-eabi-gdb --command=built/debug.gdb < /dev/null 2>&1 | tee built/flash.log
	$(V)grep -q "Start address" built/flash.log
endif
	@echo
	@echo "*** Flash OK"
	@echo

flash-loop: all
	while : ; do make rc && say done ; sleep 2 ; done

flash-loop-bl: all
	while : ; do make BL=1 r && sleep 2 && say done ; done

flash-loop-app: all
	while : ; do make r && say done ; sleep 2 ; done

prep-built-gdb:
	echo "file $(ELF)" > built/debug.gdb
ifeq ($(BMP),)
	echo "target extended-remote | $(OPENOCD) -f $(SCRIPTS)/gdbdebug.cfg" >> built/debug.gdb
else
	echo "target extended-remote $(BMP_PORT)" >> built/debug.gdb
ifneq ($(HARD_ERASE)$(SRST),)
	echo "monitor connect_srst enable" >> built/debug.gdb
else
	echo "monitor connect_srst disable" >> built/debug.gdb
endif
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
$(wildcard $(BUILT)/*/*/*/*/*/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/*/*/*/*/*/*/*.o): $(CONFIG_DEPS)
$(wildcard $(BUILT)/*/*/*/*/*/*/*/*.o): $(CONFIG_DEPS)
$(OBJ): $(CONFIG_DEPS)

$(BUILT)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo AS $<
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.hex: %.elf $(SCRIPTS)/bin2uf2.js $(SCRIPTS)/bin2c.js
	@echo BIN/HEX $<
	$(V)$(PREFIX)objcopy -O binary $< $(@:.hex=.bin)
	$(V)$(PREFIX)objcopy -O ihex $< $@
ifneq ($(BL),)
	$(V)node $(SCRIPTS)/bin2c.js $(@:.hex=.bin)
endif
ifeq ($(BL)$(NOBL),)
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

FW_VERSION = $(shell $(JD_CORE)/scripts/git-version.sh)

bump:
	sh $(SCRIPTS)/bump.sh

refresh-version:
	@mkdir -p $(BUILT_BIN)
	echo 'const char app_fw_version[] = "v$(FW_VERSION)";' > $(BUILT_BIN)/version-tmp.c
	@diff $(BUILT_BIN)/version.c $(BUILT_BIN)/version-tmp.c >/dev/null 2>/dev/null || \
		(echo "refresh version"; cp $(BUILT_BIN)/version-tmp.c $(BUILT_BIN)/version.c)

check-release: drop
	if [ "X`git describe --exact --tags --match 'v[0-9]*' 2>/dev/null`" != "X" ]; then $(MAKE) build-release ; fi

build-release:
	# avoid re-computing FW_VERSION many times
	$(MAKE) do-build-release FW_VERSION=$(FW_VERSION)

do-build-release:
	cp built/drop.uf2 dist/fw-$(FW_VERSION).uf2
	git add dist/fw-$(FW_VERSION).uf2
	if [ "X$$GITHUB_WORKFLOW" != "X" ] ; then git config user.email "<>" && git config user.name "GitHub Bot" ; fi
	git commit -m "[skip ci] firmware v$(FW_VERSION) built"
	git push

$(BUILT_BIN)/%.o: $(BUILT_BIN)/%.c
	$(V)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

ifneq ($(BLUP),)
PROF_DEP = $(BUILT_BIN)/bl
else
PROF_DEP = $(BUILT)/jd/prof
endif

$(BUILT_BIN)/$(PREF)-%.elf: $(PROF_DEP)-%.o $(OBJ) Makefile $(LD_SCRIPT) $(SCRIPTS)/patch-bin.js $(FORCE)
	@echo LD $@
ifeq ($(OS), Windows_NT)
	$(file >$(BUILT_BIN)/$(PREF)-flags.txt,$(OBJ))
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$@.map -o $@ @$(BUILT_BIN)/$(PREF)-flags.txt $< -lm
else
	$(V)$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$@.map  -o $@ $(OBJ) $< -lm
endif
ifeq ($(NOBL),)
	@echo BIN-PATCH $@
	$(V)node $(SCRIPTS)/patch-bin.js $@ $(FLASH_SIZE) $(BL_SIZE) targets/$(TARGET)/profile $(PAGE_SIZE) blup=$(BLUP)
endif

build: $(addsuffix .hex,$(addprefix $(BUILT_BIN)/$(PREF)-,$(PROFILES)))

combine: $(addsuffix .hex,$(addprefix $(BUILT_BIN)/combined-,$(PROFILES)))

$(BUILT_BIN)/combined-%.hex: $(BUILT_BIN)/$(PREF)-%.hex
	@echo COMBINE $@
	@(cat $< | grep -v '^:0.00000[51]' ; cat $(subst app-,bl-,$<)) > $@

rc: run-combined
run-combined: $(BUILT_BIN)/combined-$(PROF).hex
	$(MAKE) ELF=$(BUILT_BIN)/combined-$(PROF).hex flash

force:
	@echo forcing...

targ-%:
	$(MAKE) TARGET=$(subst targ-,,$@)
	$(MAKE) TARGET=$(subst targ-,,$@) BLUP=1

nobl-targ-%:
	$(MAKE) TARGET=$(subst nobl-targ-,,$@)

drop: $(addprefix nobl-targ-,$(NO_DROP_TARGETS)) $(addprefix targ-,$(DROP_TARGETS))
	cd built; cat $(addsuffix /app-*.uf2,$(DROP_TARGETS)) > drop.uf2
	cd built; cat $(addsuffix /blup-*.uf2,$(DROP_TARGETS)) > bootloader-update.uf2
	@ls -l built/drop.uf2 built/bootloader-update.uf2
	cp built/drop.uf2 built/fw-$(FW_VERSION).uf2

ff: full-flash

full-flash:
ifeq ($(NOBL),)
	$(MAKE) BL=1 r
endif
	$(MAKE) r

update-submodules:
	cd $(JD_STM) && git checkout main && git pull
	cd $(JD_CORE) && git checkout main && git pull
