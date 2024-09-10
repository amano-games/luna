ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

DESTDIR   ?=
PREFIX    ?=
BINDIR    ?= ${PREFIX}playdate
TARGET    := $(GAME_NAME).pdx
BUILD_DIR := ${DESTDIR}${BINDIR}

LDLIBS := -lm
LDFLAGS :=


LUA_DIR     := playdate
SDK         := ${PLAYDATE_SDK_PATH}
SIM         := $(SDK)/bin/PlaydateSimulator
SDK_SRC_DIR := $(SDK)/C_API
SRC_SDK     := $(SDK_SRC_DIR)/buildsupport/setup.c
LDSCRIPT    := $(patsubst ~%,$(HOME)%,$(SDK_SRC_DIR)/buildsupport/link_map.ld)

ifeq ($(DETECTED_OS), Linux)
	DYLIB_FLAGS := -shared -fPIC
	DYLIB_EXT   := so
endif
ifeq ($(DETECTED_OS), Darwin)
	DYLIB_FLAGS := -dynamiclib -rdynamic
	DYLIB_EXT   := dylib
endif

SIM_SRC := $(BUILD_DIR)/pdex.${DYLIB_EXT}


WATCH_SRC   := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC   += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external $(SDK_SRC_DIR)
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := $(shell find $(SRC_DIR) -type d)
INC_DIRS       += $(LUNA_DIR) $(LUNA_DIR)/sys $(LUNA_DIR)/lib $(LUNA_DIR)/core
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

CDEFS          := -DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_FLAGS  += -fsanitize=undefined -fsanitize-trap

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

OBJS         := $(BUILD_DIR)/$(TARGET)
ASSETS_OUT   := $(OBJS)/assets

PDC         := $(SDK)/bin/pdc
PDCFLAGS    := -k

# Output library names and executables.
ELF            := $(BUILD_DIR)/pdex.elf
BIN            := $(BUILD_DIR)/pdex.bin
HEX            := $(BUILD_DIR)/pdex.hex
ELF_OUT        := $(BUILD_DIR)/$(LUA_DIR)/pdex.elf

PD_CC          := arm-none-eabi-gcc
PD_OBJCOPY     := arm-none-eabi-objcopy

PD_CFLAGS      :=
PD_CFLAGS      += -mcpu=arm7tdmi
PD_CFLAGS      += -mtune=arm7tdmi
PD_CFLAGS      += -specs=nosys.specs
PD_CFLAGS      += -mword-relocations
PD_CFLAGS      += -fdata-sections
PD_CFLAGS      += -ffunction-sections
PD_CFLAGS      += -fno-strict-aliasing
PD_CFLAGS      += -fsingle-precision-constant 
PD_CFLAGS      += -fno-common

.DEFAULT_GOAL := all
.PHONY: all clean build run

clean:
	rm -rf $(BUILD_DIR)

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -r $(LUA_DIR) $(BUILD_DIR)

$(OBJS): $(BUILD_DIR)
	$(PDC) $(PDCFLAGS) $(BUILD_DIR)/$(LUA_DIR) $@

$(BIN): $(ELF)
	$(PD_OBJCOPY) -v -O binary $(ELF) $(BIN)

$(HEX): $(ELF)
	$(PD_OBJCOPY) -v -O hex $(ELF) $(HEX)

$(ELF): $(SRC_DIR)/main.c $(WATCH_SRC) $(LDSCRIPT) $(BUILD_DIR)
	$(PD_CC) $(PD_CFLAGS) $(CFLAGS) $(INC_FLAGS) \
	%< $(LDLIBS) $(LDFLAGS) -o $@

$(ELF_OUT):$(ELF) $(PD_BUILD_DIR)
	cp $< $@

$(SIM_SRC): $(SRC_DIR)/main.c $(WATCH_SRC) $(BUILD_DIR)
	$(CC) \
		$(CFLAGS) \
		$(DYLIB_FLAGS) \
		$(INC_FLAGS) \
		$< $(SRC_SDK) \
		$(LDLIBS) \
		$(LDFLAGS) -o $@
	cp $(SIM_SRC) $(BUILD_DIR)/$(LUA_DIR)

all: $(SIM_SRC) $(ASSETS_OUT) $(OBJS)
run: all
	$(LUNA_DIR)/close-sim.sh
	$(SIM) "$(abspath $(PDX))"

