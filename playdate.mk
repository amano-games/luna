space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/playdate
TARGET       := $(GAME_NAME).pdx

ifeq ($(DEBUG),0)
BINDIR ?= ${PREFIX}playdate-release
else
BINDIR ?= ${PREFIX}playdate
endif

BUILD_DIR := ${DESTDIR}${BINDIR}

LDLIBS  := -lm

SDK          := ${PLAYDATE_SDK_PATH}
SIM          := $(SDK)/bin/PlaydateSimulator
SDK_SRC_DIR  := $(SDK)/C_API
SRC_SDK      := $(SDK_SRC_DIR)/buildsupport/setup.c
LDSCRIPT     := $(patsubst ~%,$(HOME)%,$(SDK_SRC_DIR)/buildsupport/link_map.ld)

ifeq ($(DETECTED_OS), Linux)
DYLIB_FLAGS := -shared -fPIC
DYLIB_EXT   := so
endif
ifeq ($(DETECTED_OS), Darwin)
DYLIB_FLAGS := -dynamiclib -rdynamic
DYLIB_EXT   := dylib
endif

EXTERNAL_DIRS  := $(LUNA_DIR)/external $(SDK_SRC_DIR)
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS  := src $(LUNA_DIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DTARGET_EXTENSION=1 -DBACKEND_PD=1

PD_DEFS  := -DTARGET_PLAYDATE=1 -DTARGET_PD_DEVICE
SIM_DEFS := -DTARGET_SIMULATOR=1 -DTARGET_PLAYDATE=0 -DTARGET_PD_SIM

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g -gdwarf-2
RELEASE_CFLAGS += -fomit-frame-pointer
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += $(WARN_FLAGS)

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DDEBUG=1

ifeq ($(DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

OBJS         := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip
# Stage assets outside the .pdx so mkdir does not fake an up-to-date pdx target.
ASSETS_OUT := $(BUILD_DIR)/packed-assets
include $(ROOT_DIR)/assets.mk

PDC      := $(SDK)/bin/pdc
PDCFLAGS := -k

ELF     := $(BUILD_DIR)/pdex.elf
ELF_OUT := $(BUILD_DIR)/tmp/pdex.elf

PD_CC := arm-none-eabi-gcc

MCU     := cortex-m7
FPU     := -mfloat-abi=hard -mfpu=fpv5-sp-d16 -D__FPU_USED=1
MCFLAGS := -mthumb -mcpu=$(MCU) $(FPU)
ARCH    := $(MCFLAGS)

PD_CFLAGS :=
PD_CFLAGS += $(ARCH)
PD_CFLAGS += -specs=nosys.specs
PD_CFLAGS += -mword-relocations
PD_CFLAGS += -fdata-sections
PD_CFLAGS += -ffunction-sections
PD_CFLAGS += -fno-strict-aliasing
PD_CFLAGS += -fsingle-precision-constant
PD_CFLAGS += -falign-functions=32
PD_CFLAGS += -falign-loops=32
PD_CFLAGS += -fno-common
PD_CFLAGS += $(PD_DEFS)

PD_LDFLAGS := $(ARCH)
PD_LDFLAGS += -nostartfiles
PD_LDFLAGS += -T$(LDSCRIPT)
PD_LDFLAGS += -Wl,--emit-relocs,--gc-sections,--no-warn-mismatch

SIM_CFLAGS := $(SIM_DEFS)
ifeq ($(DETECTED_OS), Linux)
SIM_CFLAGS += -fsanitize-trap
endif

# Base flags for device; sim adds SIM_CFLAGS/-fPIC into CFLAGS for game.mk.
CFLAGS_BASE  := $(CFLAGS)
OBJ_DIR      := $(BUILD_DIR)/obj/sim
CFLAGS       := $(CFLAGS_BASE) $(SIM_CFLAGS) -fPIC
include $(ROOT_DIR)/game.mk

SETUP_OBJ := $(OBJ_DIR)/setup.o

# Device objects (separate tree; different CC/flags)
PD_OBJ_DIR  := $(BUILD_DIR)/obj/device
PD_LUNA_OBJ := $(PD_OBJ_DIR)/luna.o
PD_GAME_OBJ := $(PD_OBJ_DIR)/game.o
PD_SETUP_OBJ:= $(PD_OBJ_DIR)/setup.o
PD_UNITY_OBJS := $(PD_LUNA_OBJ) $(PD_GAME_OBJ) $(PD_SETUP_OBJ)

.PHONY: all clean build build_sim build_pd run assets assets_clean release publish

TMP_DIR := $(BUILD_DIR)/tmp

$(TMP_DIR):
	mkdir -p "$@"

# Keep simulator binary in tmp/ (pdc input). Stamp avoids deleting the Make target.
SIM_BIN := $(TMP_DIR)/pdex.$(DYLIB_EXT)
SIM_READY := $(TMP_DIR)/.sim_ready
DEVICE_READY := $(TMP_DIR)/.device_ready

$(SETUP_OBJ): $(SRC_SDK) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) $(DEPFLAGS) -c "$<" -o "$@"

$(SIM_READY): $(UNITY_OBJS) $(SETUP_OBJ) | $(TMP_DIR)
	$(CC) $(CFLAGS) $(DYLIB_FLAGS) $(UNITY_OBJS) $(SETUP_OBJ) $(LDLIBS) $(LDFLAGS) -o "$(SIM_BIN)"
	touch "$@"

$(PD_OBJ_DIR):
	mkdir -p "$(PD_OBJ_DIR)"

$(PD_LUNA_OBJ): $(LUNA_SRC) | $(PD_OBJ_DIR)
	$(PD_CC) $(PD_CFLAGS) $(CFLAGS_BASE) $(INC_FLAGS) $(DEPFLAGS) -c "$<" -o "$@"

$(PD_GAME_OBJ): $(GAME_SRC) | $(PD_OBJ_DIR)
	$(PD_CC) $(PD_CFLAGS) $(CFLAGS_BASE) $(INC_FLAGS) $(DEPFLAGS) -c "$<" -o "$@"

$(PD_SETUP_OBJ): $(SRC_SDK) | $(PD_OBJ_DIR)
	$(PD_CC) $(PD_CFLAGS) $(CFLAGS_BASE) $(INC_FLAGS) $(DEPFLAGS) -c "$<" -o "$@"

-include $(PD_LUNA_OBJ:.o=.d)
-include $(PD_GAME_OBJ:.o=.d)
-include $(PD_SETUP_OBJ:.o=.d)
-include $(SETUP_OBJ:.o=.d)

$(ELF): $(PD_UNITY_OBJS) $(LDSCRIPT)
	mkdir -p "$(BUILD_DIR)"
	$(PD_CC) $(PD_CFLAGS) $(CFLAGS_BASE) $(PD_UNITY_OBJS) $(LDLIBS) $(LDFLAGS) $(PD_LDFLAGS) -o "$@"

$(DEVICE_READY): $(ELF) | $(TMP_DIR)
	cp "$(ELF)" "$(ELF_OUT)"
	touch "$@"

# Package .pdx only after the required binary stamp(s) exist (safe under -j).
$(OBJS): | $(TMP_DIR)
	rm -rf "$@"
	cp -r $(PLATFORM_DIR)/* $(TMP_DIR)
	$(PDC) $(PDCFLAGS) $(TMP_DIR) "$@"
	mkdir -p "$@/assets"
	cp -a "$(ASSETS_OUT)/." "$@/assets/"
	rm -f "$@/assets/.timestamp"

all: run

build_sim: $(SIM_READY) $(ASSETS_TIMESTAMP)
	$(MAKE) -f $(ROOT_DIR)/playdate.mk $(OBJS) \
		DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) \
		PLATFORM_DIR=$(PLATFORM_DIR) DEBUG=$(DEBUG) CDEFS="$(CDEFS)" CC="$(CC)"

build_pd: $(DEVICE_READY) $(ASSETS_TIMESTAMP)
	$(MAKE) -f $(ROOT_DIR)/playdate.mk $(OBJS) \
		DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) \
		PLATFORM_DIR=$(PLATFORM_DIR) DEBUG=$(DEBUG) CDEFS="$(CDEFS)"

build: $(SIM_READY) $(DEVICE_READY) $(ASSETS_TIMESTAMP)
	$(MAKE) -f $(ROOT_DIR)/playdate.mk $(OBJS) \
		DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) \
		PLATFORM_DIR=$(PLATFORM_DIR) DEBUG=$(DEBUG) CDEFS="$(CDEFS)" CC="$(CC)"

assets_clean:
	rm -rf $(ASSETS_OUT)
assets: $(ASSETS_TIMESTAMP)

release:
	$(MAKE) -f $(ROOT_DIR)/playdate.mk clean DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) PLATFORM_DIR=$(PLATFORM_DIR)
	$(MAKE) -f $(ROOT_DIR)/playdate.mk build DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) PLATFORM_DIR=$(PLATFORM_DIR) CDEFS="$(CDEFS)"
	$(MAKE) -f $(ROOT_DIR)/playdate.mk run DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) PLATFORM_DIR=$(PLATFORM_DIR)

clean:
	rm -rf "$(BUILD_DIR)"

ifeq ($(DETECTED_OS), Linux)
run: build_sim
	$(LUNA_DIR)/close-sim.sh
	$(SIM) "$(abspath $(OBJS))"
endif

ifeq ($(DETECTED_OS), Darwin)
run: build_sim
	open "$(abspath $(OBJS))"
endif

$(PUBLISH_OBJS): $(DEVICE_READY) $(OBJS) $(ASSETS_TIMESTAMP)
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./$(TARGET)

publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):playdate
