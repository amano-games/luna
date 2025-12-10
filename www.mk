space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

CC           := emcc
DESTDIR      ?=
PREFIX       ?=
BINDIR       ?= ${PREFIX}www
TARGET       := index.html
BUILD_DIR    := ${DESTDIR}${BINDIR}
PLATFORM_DIR := platforms/www

LDLIBS := -lm
LDFLAGS :=

WATCH_SRC    := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC    += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := src
INC_DIRS       += $(LUNA_DIR)
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DBACKEND_SOKOL=1 -DTARGET_WASM=1 -DSOKOL_GLES3

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -g3
RELEASE_CFLAGS += -fomit-frame-pointer
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += -DSOKOL_DEBUG=0
RELEASE_CFLAGS += $(WARN_FLAGS)

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DDEBUG=1
DEBUG_CFLAGS += -Wno-limited-postlink-optimizations
DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable,undefined

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)
CFLAGS += -s ALLOW_MEMORY_GROWTH=1
CFLAGS += -s USE_WEBGL2
CFLAGS += -s NO_EXIT_RUNTIME=1
CFLAGS += --shell-file=$(PLATFORM_DIR)/index.html
CFLAGS += --preload-file=$(BUILD_DIR)/assets@/assets
CFLAGS += --preload-file=$(BUILD_DIR)/icons@/icons

ASSETS_OUT   := $(BUILD_DIR)/assets
OBJS         := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip

.PHONY: all clean build run publish
.DEFAULT_GOAL := all

all: clean build run

$(ASSETS_BIN): $(ASSETS_WATCH_SRC)
	make -f $(LUNA_DIR)/tools.mk tools-asset

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -r $(PLATFORM_DIR)/* $(BUILD_DIR)

$(OBJS): $(SRC_DIR)/main.c $(SHADER_OBJS) $(BUILD_DIR) $(ASSETS_OUT) $(WATCH_SRC)
	$(CC) $(CFLAGS) $(INC_FLAGS) $< $(LDLIBS) $(LDFLAGS) -o $@

$(PUBLISH_OBJS): $(OBJS)
	rm -rf $(BUILD_DIR)/assets
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./*

clean:
	rm -rf $(BUILD_DIR)

run: $(OBJS)
	emrun $(OBJS)

build: $(OBJS)
release: clean build
publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):html5
