space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

CC           := emcc
DESTDIR      ?=
PREFIX       ?=
PLATFORM_DIR := platforms/www
TARGET       := index.html

RELEASE_BINDIR := ${PREFIX}www-release
ifeq ($(DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}www
endif

BUILD_DIR := ${DESTDIR}${BINDIR}
PUBLISH_BUILD_DIR := ${DESTDIR}$(RELEASE_BINDIR)

LDLIBS := -lm
LDFLAGS :=

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

override CDEFS := $(CDEFS) -DBACKEND_SOKOL=1 -DTARGET_WASM=1 -DSOKOL_GLES3

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -g
RELEASE_CFLAGS += -fomit-frame-pointer
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += -DSOKOL_DEBUG=0
RELEASE_CFLAGS += $(WARN_FLAGS)

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DSOKOL_DEBUG=1
DEBUG_CFLAGS += -DDEBUG=1
DEBUG_CFLAGS += -Wno-limited-postlink-optimizations
DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable,undefined

ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)
LINK_FLAGS :=
LINK_FLAGS += -s ALLOW_MEMORY_GROWTH=1
LINK_FLAGS += -s USE_WEBGL2
LINK_FLAGS += -s NO_EXIT_RUNTIME=1
LINK_FLAGS += --shell-file=$(PLATFORM_DIR)/index.html
LINK_FLAGS += --preload-file=$(BUILD_DIR)/assets@/assets
LINK_FLAGS += --preload-file=$(BUILD_DIR)/icons@/icons

ASSETS_OUT   := $(BUILD_DIR)/assets
OBJ_DIR      := $(BUILD_DIR)/obj
BINARY       := $(BUILD_DIR)/$(TARGET)
PUBLISH_OBJS := $(PUBLISH_BUILD_DIR)/$(GAME_NAME).zip

include $(ROOT_DIR)/game.mk
include $(ROOT_DIR)/assets.mk

.PHONY: all clean build run publish_release release
.DEFAULT_GOAL := all

all: build run

# Directory existence is not enough: obj/assets may create BUILD_DIR first.
PLATFORM_READY := $(BUILD_DIR)/icons

$(PLATFORM_READY):
	mkdir -p $(BUILD_DIR)
	cp -r $(PLATFORM_DIR)/. $(BUILD_DIR)/

$(BINARY): $(UNITY_OBJS) | $(PLATFORM_READY) $(ASSETS_TIMESTAMP)
	$(CC) $(CFLAGS) $(LINK_FLAGS) $(UNITY_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(BINARY)
	emrun $(BINARY)

build: $(BINARY)

$(PUBLISH_OBJS): $(BINARY)
	rm -rf $(BUILD_DIR)/assets
	cd $(BUILD_DIR) && zip -r ./$(GAME_NAME).zip ./*

release:
	$(MAKE) -f $(ROOT_DIR)/www.mk clean DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME)
	$(MAKE) -f $(ROOT_DIR)/www.mk build DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"

publish_release:
	$(MAKE) -f $(ROOT_DIR)/www.mk release DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	$(MAKE) -f $(ROOT_DIR)/www.mk $(PUBLISH_OBJS) DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)"
	butler push $(PUBLISH_OBJS) $(COMPANY_NAME)/$(GAME_NAME):html5
