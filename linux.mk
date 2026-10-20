# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

# TCC=1 builds with tinycc: no sanitizers, no pedantic/-Werror flags.
TCC ?= 0
ifeq ($(TCC),1)
CC := tcc
else
CC := clang
endif

# Hot code reloading
# Names are relative to the exe dir and passed to the host as SYS_HOT_* defines
HOT             ?= 0
GAME_SO_NAME    := game.so
GAME_LOCK_NAME  := game.lock
GAME_TMP_PREFIX := obj/game-hot-
ifeq ($(HOT),1)
ifeq ($(BUILD_DEBUG),0)
$(error HOT=1 is debug only)
endif
override ASAN  := 0
override CDEFS := $(CDEFS) -DSYS_HOT_RELOAD=1 -DSYS_HOT_LIB_PATH=\"$(GAME_SO_NAME)\" -DSYS_HOT_LOCK_PATH=\"$(GAME_LOCK_NAME)\" -DSYS_HOT_TMP_PREFIX=\"$(GAME_TMP_PREFIX)\"
endif

DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/linux
TARGET       := $(APP_NAME).bin

RELEASE_BINDIR := ${PREFIX}linux-release
ifeq ($(BUILD_DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}linux
endif

BUILD_DIR := ${DESTDIR}${BINDIR}
PUBLISH_BUILD_DIR := ${DESTDIR}$(RELEASE_BINDIR)

LDLIBS := -lm -ldl -lrt -lGL -lX11 -lasound -lXi -lXcursor -lpthread
LDFLAGS :=

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)

override CDEFS := $(CDEFS) -DSOKOL_GLCORE -DSYS_GFX_SOKOL

STEAM_PLATFORM := linux64
include $(ROOT_DIR)/steamworks.mk

INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

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
DEBUG_CFLAGS += -fno-omit-frame-pointer
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DBUILD_DEBUG=1
DEBUG_CFLAGS += $(SANITIZE_FLAGS)

ifeq ($(TCC),1)
RELEASE_CFLAGS := ${CFLAGS} -g -DNDEBUG -DBUILD_DEBUG=0 -Wall
DEBUG_CFLAGS   := -g -Wall -DSOKOL_DEBUG=1 -DBUILD_DEBUG=1
SANITIZE_FLAGS :=
endif

ifeq ($(BUILD_DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
	LDFLAGS += $(SANITIZE_FLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

OBJ_DIR      := $(BUILD_DIR)/obj
BINARY       := $(BUILD_DIR)/$(TARGET)
GAME_SO      := $(BUILD_DIR)/$(GAME_SO_NAME)
GAME_LOCK    := $(BUILD_DIR)/$(GAME_LOCK_NAME)
PUBLISH_OBJS := $(PUBLISH_BUILD_DIR)/$(APP_NAME).zip

# Platform files must land before obj/assets mkdir -p creates BUILD_DIR.
ASSETS_EXTRA := $(BUILD_DIR)

include $(ROOT_DIR)/game.mk
include $(ROOT_DIR)/assets.mk

ifeq ($(HOT),1)
HOST_OBJS     := $(LUNA_OBJ)
HOST_LDFLAGS  := -rdynamic
BUILD_TARGETS := $(BINARY) $(GAME_SO)
$(GAME_OBJ): CFLAGS += -fPIC
else
HOST_OBJS     := $(UNITY_OBJS)
HOST_LDFLAGS  :=
BUILD_TARGETS := $(BINARY)
endif

.PHONY: all clean build game steam_build steam_release_build run release publish_itch
.DEFAULT_GOAL := all

all: build
	$(MAKE) -f $(ROOT_DIR)/linux.mk run DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) BUILD_DEBUG=$(BUILD_DEBUG) CDEFS='$(CDEFS)'

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -fr $(PLATFORM_DIR)/. $(BUILD_DIR)

$(BINARY): $(HOST_OBJS) | $(BUILD_DIR) assets
	$(CC) $(CFLAGS) $(HOST_OBJS) $(LDLIBS) $(LDFLAGS) $(HOST_LDFLAGS) -o $@

$(GAME_SO): $(GAME_OBJ) | $(BUILD_DIR)
	touch "$(GAME_LOCK)"; \
	$(CC) -shared $(CFLAGS) $(GAME_OBJ) -lm $(LDFLAGS) -o "$@.tmp" && mv -f "$@.tmp" "$@"; \
	rc=$$?; rm -f "$(GAME_LOCK)"; exit $$rc

$(PUBLISH_OBJS): $(BINARY)
	rm -rf $(BUILD_DIR)/assets
	rm -rf $(BUILD_DIR)/obj
	cd $(BUILD_DIR) && zip -r ./$(APP_NAME).zip ./*

clean:
	rm -rf $(BUILD_DIR)

build:
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(BUILD_TARGETS) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'

game:
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(GAME_SO) assets DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'

STEAM_LIB_OUT := $(dir $(BINARY))$(notdir $(STEAM_LIB))

# Using install for hot code reloading safe shananigans
$(STEAM_LIB_OUT): $(STEAM_LIB) | $(BUILD_DIR)
	install -m 644 "$<" "$@"

steam_build:
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(BUILD_TARGETS) $(STEAM_LIB_OUT) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
ifeq ($(BUILD_DEBUG),1)
	printf '%s\n' "$(STEAM_APP_ID)" > "$(dir $(BINARY))steam_appid.txt"
endif

run:
	cd $(BUILD_DIR) && ./$(TARGET)

release:
	$(MAKE) -f $(ROOT_DIR)/linux.mk clean BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME)
	$(MAKE) -f $(ROOT_DIR)/linux.mk build BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/assets
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/obj

steam_release_build:
	$(MAKE) -f $(ROOT_DIR)/linux.mk clean BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME)
	$(MAKE) -f $(ROOT_DIR)/linux.mk steam_build BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/assets
	rm -rf $(DESTDIR)$(RELEASE_BINDIR)/obj

publish_itch:
	$(MAKE) -f $(ROOT_DIR)/linux.mk release DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(PUBLISH_OBJS) BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	butler push $(PUBLISH_OBJS) $(ITCH_ORG_NAME)/$(ITCH_APP_NAME):linux
