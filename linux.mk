# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

CC := clang

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

ifeq ($(BUILD_DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
	LDFLAGS += $(SANITIZE_FLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

OBJ_DIR      := $(BUILD_DIR)/obj
BINARY       := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(PUBLISH_BUILD_DIR)/$(APP_NAME).zip

# Platform files must land before obj/assets mkdir -p creates BUILD_DIR.
ASSETS_EXTRA := $(BUILD_DIR)

include $(ROOT_DIR)/game.mk
include $(ROOT_DIR)/assets.mk

.PHONY: all clean build steam_build steam_release_build run release publish_itch
.DEFAULT_GOAL := all

all: build
	$(MAKE) -f $(ROOT_DIR)/linux.mk run DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) BUILD_DEBUG=$(BUILD_DEBUG) CDEFS='$(CDEFS)'

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -fr $(PLATFORM_DIR)/. $(BUILD_DIR)

$(BINARY): $(UNITY_OBJS) | $(BUILD_DIR) assets
	$(CC) $(CFLAGS) $(UNITY_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

$(PUBLISH_OBJS): $(BINARY)
	rm -rf $(BUILD_DIR)/assets
	rm -rf $(BUILD_DIR)/obj
	cd $(BUILD_DIR) && zip -r ./$(APP_NAME).zip ./*

clean:
	rm -rf $(BUILD_DIR)

build:
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(BINARY) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'

steam_build:
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(BINARY) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) APP_NAME=$(APP_NAME) CDEFS='$(CDEFS)'
	cp -f "$(STEAM_LIB)" "$(dir $(BINARY))"
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
