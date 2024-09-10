space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

DESTDIR      ?=
PREFIX       ?=
BINDIR       ?= ${PREFIX}linux
TARGET       := $(GAME_NAME).bin
BUILD_DIR    := ${DESTDIR}${BINDIR}
PLATFORM_DIR := platforms/linux

LDLIBS := -lm -ldl -lrt -lGL -lX11 -lasound -lXi -lXcursor -lpthread
LDFLAGS :=

WATCH_SRC   := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC   += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external $(LUNA_DIR)/external/sokol
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := $(shell find $(SRC_DIR) -type d)
INC_DIRS       += $(LUNA_DIR) $(LUNA_DIR)/sys $(LUNA_DIR)/lib $(LUNA_DIR)/core
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

CDEFS        := -DBACKEND_SOKOL=1 -DSOKOL_DEBUG=1 -DSOKOL_GLCORE33

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
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip
RPATH        := '-Wl,-z,origin -Wl,-rpath,$$ORIGIN/steam-runtime/amd64/lib/x86_64-linux-gnu:$$ORIGIN/steam-runtime/amd64/lib:$$ORIGIN/steam-runtime/amd64/usr/lib/x86_64-linux-gnu:$$ORIGIN/steam-runtime/amd64/usr/lib'

.PHONY: all clean build steam run
.DEFAULT_GOAL := all

all: clean build run

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -r $(PLATFORM_DIR)/* $(BUILD_DIR)

$(OBJS): $(SRC_DIR)/main.c $(SHADER_OBJS) $(BUILD_DIR) $(ASSETS_OUT) $(WATCH_SRC)
	$(CC) $(CFLAGS) $(INC_FLAGS) $< $(LDLIBS) $(LDFLAGS) -o $@

$(BUILD_DIR)/steam-runtime:
	$(LUNA_DIR)/update_runtime.sh
	$(LUNA_DIR)/extract_runtime.sh $(ROOT_DIR)/steam-runtime-release_latest.tar.xz amd64 $(BUILD_DIR)/steam-runtime

$(PUBLISH_OBJS): $(OBJS) steam
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./*

steam: $(BUILD_DIR)/steam-runtime

clean:
	rm -rf $(BUILD_DIR)

run: build
	cd $(BUILD_DIR) && ./$(TARGET)

build: $(OBJS)
release: clean $(PUBLISH_OBJS)
publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):linux
