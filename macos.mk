space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

DESTDIR      ?=
PREFIX       ?=
BINDIR       ?= ${PREFIX}macos/$(GAME_NAME).app
TARGET       := $(GAME_NAME)
BUILD_DIR    := ${DESTDIR}${BINDIR}
PLATFORM_DIR := platforms/macos

LDLIBS := -lm -framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit -framework AudioToolbox
LDFLAGS :=

WATCH_SRC   := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC   += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external $(LUNA_DIR)/external/sokol
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := $(shell find $(SRC_DIR) -type d)
INC_DIRS       += $(LUNA_DIR) $(LUNA_DIR)/sys $(LUNA_DIR)/lib $(LUNA_DIR)/core
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DBACKEND_SOKOL=1 -DSOKOL_DEBUG=1 -DSOKOL_METAL -DTARGET_MACOS

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DDEBUG=1
# DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable

DEBUG ?= 0
ifeq ($(DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS) -ObjC -x objective-c

# TODO: Move assets to resources
ASSETS_OUT   := $(BUILD_DIR)/Contents/Resources/assets
OBJS         := $(BUILD_DIR)/Contents/MacOS/$(TARGET)
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip

.PHONY: all clean build run publish
.DEFAULT_GOAL := all

all: clean build run

$(ASSETS_OUT): $(ASSETS_BIN)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/Contents/MacOS
	mkdir -p $(BUILD_DIR)/Contents/Resources
	cp -r $(PLATFORM_DIR)/Info.plist $(BUILD_DIR)/Contents
	cp -r $(PLATFORM_DIR)/Resources/* $(BUILD_DIR)/Contents/Resources

$(OBJS): $(SRC_DIR)/main.c $(SHADER_OBJS) $(BUILD_DIR) $(ASSETS_OUT) $(WATCH_SRC)
	$(CC) $(CFLAGS) $(INC_FLAGS) $< $(LDLIBS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

run: build
	open $(BUILD_DIR)

build: $(OBJS)
release: clean $(PUBLISH_OBJS)
publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):macos
