space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/linux
TARGET       := $(GAME_NAME).bin

RELEASE_BINDIR := ${PREFIX}linux-release
ifeq ($(DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}linux
endif

BUILD_DIR := ${DESTDIR}${BINDIR}
PUBLISH_BUILD_DIR := ${DESTDIR}$(RELEASE_BINDIR)

LDLIBS := -lm -ldl -lrt -lGL -lX11 -lasound -lXi -lXcursor -lpthread
RPATH  := '-Wl,-z,origin -Wl,-rpath,$$ORIGIN/steam-runtime/amd64/lib/x86_64-linux-gnu:$$ORIGIN/steam-runtime/amd64/lib:$$ORIGIN/steam-runtime/amd64/usr/lib/x86_64-linux-gnu:$$ORIGIN/steam-runtime/amd64/usr/lib'
LDFLAGS := $(RPATH)

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DBACKEND_SOKOL=1 -DSOKOL_GLCORE -DTARGET_LINUX

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += $(WARN_FLAGS)
RELEASE_CFLAGS += -fno-omit-frame-pointer

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += -fno-omit-frame-pointer
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DDEBUG=1

ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

ASSETS_OUT   := $(BUILD_DIR)/assets
OBJ_DIR      := $(BUILD_DIR)/obj
BINARY       := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(PUBLISH_BUILD_DIR)/$(GAME_NAME).zip

include $(ROOT_DIR)/game.mk
include $(ROOT_DIR)/assets.mk

.PHONY: all clean build steam run release publish

all: build run

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -fr $(PLATFORM_DIR)/. $(BUILD_DIR)

$(BINARY): $(UNITY_OBJS) | $(BUILD_DIR) $(ASSETS_TIMESTAMP)
	$(CC) $(CFLAGS) $(UNITY_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

$(BUILD_DIR)/steam-runtime:
	$(LUNA_DIR)/update_runtime.sh
	$(LUNA_DIR)/extract_runtime.sh $(ROOT_DIR)/steam-runtime-release_latest.tar.xz amd64 $(BUILD_DIR)/steam-runtime

$(PUBLISH_OBJS): $(BINARY) steam
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./*

steam: $(BUILD_DIR)/steam-runtime

clean:
	rm -rf $(BUILD_DIR)

build: $(BINARY)

run: build
	cd $(BUILD_DIR) && ./$(TARGET)

release:
	$(MAKE) -f $(ROOT_DIR)/linux.mk clean DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME)
	$(MAKE) -f $(ROOT_DIR)/linux.mk build DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"

publish:
	$(MAKE) -f $(ROOT_DIR)/linux.mk release DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	$(MAKE) -f $(ROOT_DIR)/linux.mk $(PUBLISH_OBJS) DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):linux
