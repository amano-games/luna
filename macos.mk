space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

# https://developer.apple.com/documentation/bundleresources/information-property-list
# https://tmewett.com/making-macos-bundle-info-plist/

DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/macos
TARGET       := $(GAME_NAME).app

RELEASE_BINDIR := ${PREFIX}macos-release
ifeq ($(DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}macos
endif

BUILD_DIR := ${DESTDIR}${BINDIR}
PUBLISH_BUILD_DIR := ${DESTDIR}$(RELEASE_BINDIR)

LDLIBS := -lm -framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit -framework AudioToolbox
LDFLAGS :=
LDLIBS += -framework IOKit -framework CoreFoundation

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DSOKOL_DEBUG=1 -DSOKOL_METAL -DTARGET_MACOS

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += $(WARN_FLAGS)
RELEASE_CFLAGS += -fno-omit-frame-pointer

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DDEBUG=1
DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable,undefined

ifeq ($(DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS) -ObjC -x objective-c -arch x86_64 -arch arm64

OBJS         := $(BUILD_DIR)/$(TARGET)
ASSETS_OUT   := $(OBJS)/Contents/Resources/assets
EXE_OUT      := $(OBJS)/Contents/MacOS/$(GAME_NAME)
PUBLISH_OBJS := $(PUBLISH_BUILD_DIR)/$(GAME_NAME).zip
OBJ_DIR      := $(BUILD_DIR)/obj

include $(ROOT_DIR)/game.mk

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJS): $(BUILD_DIR)
	mkdir -p $(OBJS)/Contents/MacOS

# App bundle dirs must exist before packing into Resources/assets.
ASSETS_EXTRA := $(BUILD_DIR) $(OBJS)
include $(ROOT_DIR)/assets.mk

.PHONY: all clean build run publish_release release sign
.DEFAULT_GOAL := all

all: build
	$(MAKE) -f $(ROOT_DIR)/macos.mk run DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) DEBUG=$(DEBUG) CDEFS="$(CDEFS)"

$(EXE_OUT): $(UNITY_OBJS) assets
	cp -r $(PLATFORM_DIR)/Info.plist $(BUILD_DIR)/$(TARGET)/Contents
	cp -r $(PLATFORM_DIR)/Resources/* $(BUILD_DIR)/$(TARGET)/Contents/Resources
	cp -r $(PLATFORM_DIR)/icons $(BUILD_DIR)/$(TARGET)/Contents/Resources
	$(CC) $(CFLAGS) $(UNITY_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

sign: $(OBJS)
	codesign --force --deep -s - $(OBJS)

$(PUBLISH_OBJS): $(EXE_OUT) sign
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

ifeq ($(DEBUG), 1)
run:
	./$(EXE_OUT)
else
run:
	open $(OBJS)
endif

build:
	$(MAKE) -f $(ROOT_DIR)/macos.mk clean DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME)
	$(MAKE) -f $(ROOT_DIR)/macos.mk $(EXE_OUT) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"

release:
	$(MAKE) -f $(ROOT_DIR)/macos.mk build DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"

publish_release:
	$(MAKE) -f $(ROOT_DIR)/macos.mk release DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	$(MAKE) -f $(ROOT_DIR)/macos.mk $(PUBLISH_OBJS) DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):macos
