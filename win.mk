space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

ifeq ($(DETECTED_OS), Linux)
CC           := x86_64-w64-mingw32-cc
endif
ifeq ($(DETECTED_OS), Darwin)
CC           := x86_64-w64-mingw32-gcc
endif
DESTDIR      ?=
PREFIX       ?=
BINDIR       ?= ${PREFIX}win
TARGET       := $(GAME_NAME).exe
BUILD_DIR    := ${DESTDIR}${BINDIR}
PLATFORM_DIR := platforms/win

LDLIBS := -lm -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
LDFLAGS :=

WATCH_SRC   := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC   += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := src
INC_DIRS       += $(LUNA_DIR)
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -mwin32 -DBACKEND_SOKOL=1 -DSOKOL_D3D11 -DTARGET_WIN

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g3
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += $(WARN_FLAGS)
RELEASE_CFLAGS += -fno-omit-frame-pointer

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DDEBUG=1
# DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable

DEBUG ?= 0
ifeq ($(DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

ASSETS_OUT   := $(BUILD_DIR)/assets
OBJS         := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip

.PHONY: all clean build steam run
.DEFAULT_GOAL := all

all: clean build run

$(ASSETS_BIN):
	make -f $(LUNA_DIR)/tools.mk tools-asset

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -fr $(PLATFORM_DIR)/. $(BUILD_DIR)

$(OBJS): $(SRC_DIR)/main.c $(SHADER_OBJS) $(BUILD_DIR) $(ASSETS_OUT) $(WATCH_SRC)
	$(CC) $(CFLAGS) $(INC_FLAGS) $< $(LDLIBS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

run: build
	cd $(BUILD_DIR) && wine ./$(TARGET)

$(PUBLISH_OBJS): $(OBJS)
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./*

build: $(OBJS)
release: clean $(PUBLISH_OBJS)
publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):win
