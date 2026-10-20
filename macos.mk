# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

CC := clang

# Hot code reloading
# Names are relative to the exe dir (Contents/MacOS) and passed to the host as SYS_HOT_* defines
HOT             ?= 0
GAME_SO_NAME    := game.so
GAME_LOCK_NAME  := game.lock
GAME_TMP_PREFIX := hot/game-hot-

ifeq ($(HOT),1)
ifeq ($(BUILD_DEBUG),0)
$(error HOT=1 is debug only)
endif
override CDEFS := $(CDEFS) \
	-DSYS_HOT_RELOAD=1 \
	-DSYS_HOT_LIB_PATH=\"$(GAME_SO_NAME)\" \
	-DSYS_HOT_LOCK_PATH=\"$(GAME_LOCK_NAME)\" \
	-DSYS_HOT_TMP_PREFIX=\"$(GAME_TMP_PREFIX)\"
endif

# https://developer.apple.com/documentation/bundleresources/information-property-list
# https://tmewett.com/making-macos-bundle-info-plist/

DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/macos
TARGET       := $(APP_NAME).app

RELEASE_BINDIR := ${PREFIX}macos-release
ifeq ($(BUILD_DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}macos
endif

BUILD_DIR := ${DESTDIR}${BINDIR}
PUBLISH_BUILD_DIR := ${DESTDIR}$(RELEASE_BINDIR)

LDLIBS := -lm -framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit -framework AudioToolbox
LDLIBS += -framework IOKit -framework CoreFoundation
LDFLAGS :=

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)

override CDEFS := $(CDEFS) -DSOKOL_DEBUG=1 -DSOKOL_METAL -DSYS_GFX_SOKOL

STEAM_PLATFORM := osx
include $(ROOT_DIR)/steamworks.mk

INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

# Local debug builds target the host
# release builds support both Mac architectures.
ifeq ($(BUILD_DEBUG), 1)
ARCH_FLAGS := -arch $(shell uname -m)
else
ARCH_FLAGS := -arch x86_64 -arch arm64
endif

SANITIZE_FLAGS :=
ifeq ($(ASAN), 1)
SANITIZE_FLAGS := -fsanitize-trap -fsanitize=address,unreachable,undefined
endif

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += -DBUILD_DEBUG=0
RELEASE_CFLAGS += $(WARN_FLAGS)
RELEASE_CFLAGS += -fno-omit-frame-pointer

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DBUILD_DEBUG=1
DEBUG_CFLAGS += $(SANITIZE_FLAGS)

ifeq ($(BUILD_DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
LDFLAGS += $(SANITIZE_FLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

# -x objective-c is compile-only; arch/sanitize also go on the link line via LDFLAGS.
CFLAGS += $(CDEFS) -x objective-c $(ARCH_FLAGS)
LDFLAGS += $(ARCH_FLAGS)

OBJS           := $(BUILD_DIR)/$(TARGET)
ASSETS_INSTALL := $(OBJS)/Contents/Resources/assets.pck
EXE_OUT        := $(OBJS)/Contents/MacOS/$(APP_NAME)
PUBLISH_OBJS   := $(PUBLISH_BUILD_DIR)/$(APP_NAME).zip
OBJ_DIR        := $(BUILD_DIR)/obj
EXE_DIR        := $(dir $(EXE_OUT))
GAME_SO        := $(EXE_DIR)$(GAME_SO_NAME)
GAME_LOCK      := $(EXE_DIR)$(GAME_LOCK_NAME)
GAME_TMP_DIR   := $(patsubst %/,%,$(EXE_DIR)$(dir $(GAME_TMP_PREFIX)))

include $(ROOT_DIR)/game.mk

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJS): $(BUILD_DIR)
	mkdir -p $(OBJS)/Contents/MacOS

# App bundle dirs must exist before installing assets.pck into Resources.
ASSETS_EXTRA := $(BUILD_DIR) $(OBJS)
include $(ROOT_DIR)/assets.mk

# HOT=1 rebuilds hold game.lock until done
# Sub-makes get GAME_LOCK_HELD=1 so only the outermost make takes the lock
GAME_LOCK_HELD  ?= 0
GAME_LOCK_BEGIN  =
GAME_LOCK_END    =

ifeq ($(HOT),1)
ifeq ($(GAME_LOCK_HELD),0)
GAME_LOCK_BEGIN  = mkdir -p "$(EXE_DIR)"; touch "$(GAME_LOCK)";
GAME_LOCK_END    = ; rc=$$?; rm -f "$(GAME_LOCK)"; exit $$rc
endif
endif

# The game is an MH_BUNDLE linked against the host with -bundle_loader
# The host exports its globals by default, so it needs no -rdynamic
ifeq ($(HOT),1)
HOST_OBJS     := $(LUNA_OBJ)
BUILD_TARGETS := $(EXE_OUT) $(GAME_SO)
else
HOST_OBJS     := $(UNITY_OBJS)
BUILD_TARGETS := $(EXE_OUT)
endif

.PHONY: all clean build game steam_build steam_release_build run publish_itch release sign
.DEFAULT_GOAL := all

all: build
	$(MAKE) -f $(ROOT_DIR)/macos.mk run DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) BUILD_DEBUG=$(BUILD_DEBUG) CDEFS='$(CDEFS)'

$(EXE_OUT): $(HOST_OBJS) | assets
	cp -r $(PLATFORM_DIR)/Info.plist $(BUILD_DIR)/$(TARGET)/Contents
	cp -r $(PLATFORM_DIR)/Resources/* $(BUILD_DIR)/$(TARGET)/Contents/Resources
	cp -r $(PLATFORM_DIR)/icons $(BUILD_DIR)/$(TARGET)/Contents/Resources
	$(CC) $(HOST_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

$(GAME_TMP_DIR): | $(OBJS)
	mkdir -p "$@"

$(GAME_SO): $(GAME_OBJ) | $(EXE_OUT) $(GAME_TMP_DIR)
	$(GAME_LOCK_BEGIN) \
	$(CC) -bundle -bundle_loader "$(EXE_OUT)" $(GAME_OBJ) $(LDFLAGS) -o "$@.tmp" && mv -f "$@.tmp" "$@" \
	$(GAME_LOCK_END)

sign: $(OBJS)
	codesign --force --deep -s - $(OBJS)

$(PUBLISH_OBJS): $(EXE_OUT) sign
	cd $(BUILD_DIR) && zip -r ./$(APP_NAME).zip ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

ifeq ($(BUILD_DEBUG), 1)
run:
	./$(EXE_OUT)
else
run:
	open $(OBJS)
endif

build:
	$(GAME_LOCK_BEGIN) \
	$(MAKE) -f $(ROOT_DIR)/macos.mk $(BUILD_TARGETS) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)' GAME_LOCK_HELD=1 \
	$(GAME_LOCK_END)

game:
	$(GAME_LOCK_BEGIN) \
	$(MAKE) -f $(ROOT_DIR)/macos.mk $(GAME_SO) assets DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)' GAME_LOCK_HELD=1 \
	$(GAME_LOCK_END)

STEAM_LIB_OUT := $(dir $(EXE_OUT))$(notdir $(STEAM_LIB))

$(STEAM_LIB_OUT): $(STEAM_LIB) | $(EXE_OUT)
	install -m 644 "$<" "$@"

steam_build:
	$(GAME_LOCK_BEGIN) \
	$(MAKE) -f $(ROOT_DIR)/macos.mk $(BUILD_TARGETS) $(STEAM_LIB_OUT) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)' GAME_LOCK_HELD=1 \
	$(GAME_LOCK_END)
ifeq ($(BUILD_DEBUG),1)
	printf '%s\n' "$(STEAM_APP_ID)" > "$(dir $(EXE_OUT))steam_appid.txt"
endif

release:
	$(MAKE) -f $(ROOT_DIR)/macos.mk clean BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME)
	$(MAKE) -f $(ROOT_DIR)/macos.mk build BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/assets
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/obj
	rm -f $(DESTDIR)$(RELEASE_BINDIR)/assets.pck

steam_release_build:
	$(MAKE) -f $(ROOT_DIR)/macos.mk clean BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME)
	$(MAKE) -f $(ROOT_DIR)/macos.mk steam_build BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/assets
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/obj
	rm -f $(DESTDIR)$(RELEASE_BINDIR)/assets.pck

publish_itch:
	$(MAKE) -f $(ROOT_DIR)/macos.mk release DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	$(MAKE) -f $(ROOT_DIR)/macos.mk $(PUBLISH_OBJS) BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	butler push $(PUBLISH_OBJS) $(ITCH_ORG_NAME)/$(ITCH_APP_NAME):macos
