space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

DESTDIR      ?=
PREFIX       ?=
BINDIR       ?= ${PREFIX}playdate
TARGET       := $(GAME_NAME).pdx
BUILD_DIR    := ${DESTDIR}${BINDIR}
PLATFORM_DIR := platforms/playdate

LDLIBS  := -lm
LDFLAGS :=

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

SIM_OUT      := $(BUILD_DIR)/pdex.${DYLIB_EXT}

WATCH_SRC    := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC    += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external $(SDK_SRC_DIR)
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := src
INC_DIRS       += $(LUNA_DIR)
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DTARGET_EXTENSION=1 -DBACKEND_PD=1

PD_DEFS        := -DTARGET_PLAYDATE=1
SIM_DEFS       := -DTARGET_SIMULATOR=1 -DTARGET_PLAYDATE=0

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g3 -gdwarf-2
RELEASE_CFLAGS += -fomit-frame-pointer
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += $(WARN_FLAGS)

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DDEBUG=1

DEBUG ?= 0
ifeq ($(DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

OBJS         := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip
ASSETS_OUT   := $(OBJS)/assets

PDC         := $(SDK)/bin/pdc
PDCFLAGS    := -k

# Output library names and executables.
ELF            := $(BUILD_DIR)/pdex.elf
ELF_OUT        := $(BUILD_DIR)/tmp/pdex.elf

PD_CC          := arm-none-eabi-gcc

PD_CFLAGS      :=
PD_CFLAGS      += -mcpu=arm7tdmi
PD_CFLAGS      += -mtune=arm7tdmi
PD_CFLAGS      += -specs=nosys.specs
PD_CFLAGS      += -mword-relocations
PD_CFLAGS      += -fdata-sections
PD_CFLAGS      += -ffunction-sections
PD_CFLAGS      += -fno-strict-aliasing
PD_CFLAGS      += -fsingle-precision-constant
PD_CFLAGS      += -falign-functions=32
PD_CFLAGS      += -falign-loops=32
PD_CFLAGS      += -fno-common
PD_CFLAGS      += $(PD_DEFS)


MCU            := cortex-m7
FPU            := -mfloat-abi=hard
FPU            += -mfpu=fpv5-sp-d16
FPU            += -D__FPU_USED=1
MCFLAGS        := -mthumb -mcpu=$(MCU) $(FPU)
ARCH           := $(MCFLAGS)

PD_LDFLAGS     := $(ARCH)
PD_LDFLAGS     += -nostartfiles
PD_LDFLAGS     += -T$(LDSCRIPT)
PD_LDFLAGS     += -Wl,--emit-relocs

SIM_CFLAGS     :=
SIM_CFLAGS     += $(SIM_DEFS)
ifeq ($(DETECTED_OS), Linux)
SIM_CFLAGS     += -fsanitize-trap -fsanitize=address,unreachable
endif

.DEFAULT_GOAL := all
.PHONY: all clean build run

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p "$(ASSETS_OUT)"
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/tmp

$(OBJS): $(BUILD_DIR)
	cp -r $(PLATFORM_DIR)/* $(BUILD_DIR)/tmp
	$(PDC) $(PDCFLAGS) $(BUILD_DIR)/tmp "$@"

$(ELF): $(SRC_DIR)/main.c $(WATCH_SRC) $(LDSCRIPT) $(BUILD_DIR)
	$(PD_CC) \
		$(PD_CFLAGS) \
		$(CFLAGS) \
		$(INC_FLAGS) \
		"$<" $(SRC_SDK) \
		$(LDLIBS) \
		$(LDFLAGS) \
		$(PD_LDFLAGS) \
		-o "$@"

$(ELF_OUT):$(ELF) $(PD_BUILD_DIR)
	cp "$<" "$@"

$(SIM_OUT): $(SRC_DIR)/main.c $(WATCH_SRC) $(BUILD_DIR)
	$(CC) \
		$(CFLAGS) \
		$(SIM_CFLAGS) \
		$(DYLIB_FLAGS) \
		$(INC_FLAGS) \
		"$<" $(SRC_SDK) \
		$(LDLIBS) \
		$(LDFLAGS) -o "$@"
	cp $(SIM_OUT) $(BUILD_DIR)/tmp
	rm $(SIM_OUT)

all: run
build: $(SIM_OUT) $(ELF_OUT) $(OBJS) $(ASSETS_OUT)
build_sim: $(SIM_OUT) $(OBJS) $(ASSETS_OUT)
build_pd: $(ELF_OUT) $(OBJS) $(ASSETS_OUT)
assets_clean:
	rm -rf $(ASSETS_OUT)
assets: $(ASSETS_OUT)

release: clean $(SIM_OUT) $(ELF_OUT) $(OBJS) $(ASSETS_OUT) run

clean:
	rm -rf "$(BUILD_DIR)"

ifeq ($(DETECTED_OS), Linux)
run: build_sim
	$(LUNA_DIR)/close-sim.sh
	LD_PRELOAD=/usr/lib/libasan.so $(SIM) "$(abspath $(OBJS))"
endif

ifeq ($(DETECTED_OS), Darwin)
run: build_sim
	open "$(abspath $(OBJS))"
endif

$(PUBLISH_OBJS): clean $(ELF_OUT) $(OBJS) $(ASSETS_OUT)
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./$(TARGET)

publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):playdate
