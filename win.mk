# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

ifeq ($(DETECTED_OS), Linux)
CC           := x86_64-w64-mingw32-cc
endif
ifeq ($(DETECTED_OS), Darwin)
CC           := x86_64-w64-mingw32-gcc
endif
DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/win
TARGET       := $(GAME_NAME).exe

RELEASE_BINDIR := ${PREFIX}win-release
ifeq ($(BUILD_DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}win
endif

BUILD_DIR := ${DESTDIR}${BINDIR}
PUBLISH_BUILD_DIR := ${DESTDIR}$(RELEASE_BINDIR)

LDLIBS := -lm -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
LDFLAGS :=

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -mwin32 -DSOKOL_D3D11 -DSYS_GFX_SOKOL

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

ifeq ($(BUILD_DEBUG), 1)
CFLAGS := $(DEBUG_CFLAGS)
else
CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += -static -static-libgcc -static-libstdc++ -lwinpthread
CFLAGS += $(CDEFS)

ASSETS_OUT   := $(BUILD_DIR)/assets
OBJ_DIR      := $(BUILD_DIR)/obj
BINARY       := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(PUBLISH_BUILD_DIR)/$(GAME_NAME).zip

include $(ROOT_DIR)/game.mk
include $(ROOT_DIR)/assets.mk

.PHONY: all clean build steam run release publish_release
.DEFAULT_GOAL := all

all: build
	$(MAKE) -f $(ROOT_DIR)/win.mk run DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) BUILD_DEBUG=$(BUILD_DEBUG) CDEFS="$(CDEFS)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -fr $(PLATFORM_DIR)/. $(BUILD_DIR)

$(BINARY): $(UNITY_OBJS) | $(BUILD_DIR) assets
	$(CC) $(CFLAGS) $(UNITY_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

run:
	cd $(BUILD_DIR) && wine ./$(TARGET)

$(PUBLISH_OBJS): $(BINARY)
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./*

build:
	$(MAKE) -f $(ROOT_DIR)/win.mk clean DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME)
	$(MAKE) -f $(ROOT_DIR)/win.mk $(BINARY) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"

release:
	$(MAKE) -f $(ROOT_DIR)/win.mk build BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"

publish_release:
	$(MAKE) -f $(ROOT_DIR)/win.mk release DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	$(MAKE) -f $(ROOT_DIR)/win.mk $(PUBLISH_OBJS) BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):win
