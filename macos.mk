space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

# https://developer.apple.com/documentation/bundleresources/information-property-list
# https://tmewett.com/making-macos-bundle-info-plist/

DESTDIR      ?=
PREFIX       ?=
BINDIR       ?= ${PREFIX}macos
TARGET       := $(GAME_NAME).app
BUILD_DIR    := ${DESTDIR}${BINDIR}
PLATFORM_DIR := platforms/macos

LDLIBS := -lm -framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit -framework AudioToolbox
LDFLAGS :=

WATCH_SRC   := $(shell find $(SRC_DIR) -name *.c -or -name *.s -or -name *.h)
WATCH_SRC   += $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))

INC_DIRS       := src
INC_DIRS       += $(LUNA_DIR)
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DBACKEND_SOKOL=1 -DSOKOL_DEBUG=1 -DSOKOL_METAL -DTARGET_MACOS

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g3
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += $(WARN_FLAGS)
RELEASE_CFLAGS += -fno-omit-frame-pointer

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DDEBUG=1
DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable

DEBUG ?= 0
ifeq ($(DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS) -ObjC -x objective-c

# TODO: Move assets to resources
OBJS         := $(BUILD_DIR)/$(TARGET)
ASSETS_OUT   := $(OBJS)/Contents/Resources/assets
EXE_OUT      := $(OBJS)/Contents/MacOS/$(GAME_NAME)
PUBLISH_OBJS := $(BUILD_DIR)/$(GAME_NAME).zip

.PHONY: all clean build run publish

all: clean build run

$(ASSETS_BIN): $(ASSETS_WATCH_SRC)
	make -f $(LUNA_DIR)/tools.mk tools-asset

$(ASSETS_OUT): $(ASSETS_BIN) $(BUILD_DIR) $(OBJS)
	mkdir -p $(ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJS): $(BUILD_DIR)
	mkdir -p $(OBJS)/Contents/MacOS

$(EXE_OUT): $(SRC_DIR)/main.c $(SHADER_OBJS) $(ASSETS_OUT) $(WATCH_SRC)
	cp -r $(PLATFORM_DIR)/Info.plist $(BUILD_DIR)/$(TARGET)/Contents
	cp -r $(PLATFORM_DIR)/Resources/* $(BUILD_DIR)/$(TARGET)/Contents/Resources
	cp -r $(PLATFORM_DIR)/icons $(BUILD_DIR)/$(TARGET)/Contents/Resources
	$(CC) $(CFLAGS) $(INC_FLAGS) $< $(LDLIBS) $(LDFLAGS) -o $@

sign: $(OBJS)
	codesign --force --deep -s - $(OBJS)

$(PUBLISH_OBJS): $(EXE_OUT) sign
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)


ifeq ($(DEBUG), 1)
run: build
	./$(EXE_OUT)
else
run: build
	open $(OBJS)
endif

build: $(EXE_OUT)
release: clean $(PUBLISH_OBJS)
publish: $(PUBLISH_OBJS)
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):macos
