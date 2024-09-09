ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

DESTDIR   ?=
PREFIX    ?=
BINDIR    ?= ${PREFIX}playdate
TARGET    := $(GAME_NAME).pdx
BUILD_DIR := ${DESTDIR}${BINDIR}

LDLIBS := -lm
LDFLAGS :=

WATCH_SRC   := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC   += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := $(shell find $(SRC_DIR) -type d)
INC_DIRS       += $(LUNA_DIR) $(LUNA_DIR)/sys $(LUNA_DIR)/lib $(LUNA_DIR)/core
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

CDEFS        :=

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

ASSETS_OUT   := $(BUILD_DIR)/assets
OBJS         := $(BUILD_DIR)/$(TARGET)

PDCFLAGS    := -k
PDC         := $(SDK)/bin/pdc

LUA_DIR     := Source
SDK         := ${PLAYDATE_SDK_PATH}
SIM         := $(SDK)/bin/PlaydateSimulator
SDK_SRC_DIR := $(SDK)/C_API
SRC_SDK     := $(SDK_SRC_DIR)/buildsupport/setup.c
LDSCRIPT    := $(patsubst ~%,$(HOME)%,$(SDK_SRC_DIR)/buildsupport/link_map.ld)

clean:
	rm -rf $(BUILD_DIR)

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

pdc: $(BUILD_DIR)
	$(PDC) $(PDCFLAGS) $(BUILD_DIR)/$(LUA_DIR) $(TARGET)
