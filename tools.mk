# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

SRC_DIR   := $(ROOT_DIR)/tools
DESTDIR   ?=
BINDIR    ?= bin
BUILD_DIR := ${BINDIR}

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS       := $(SRC_DIR) $(LUNA_DIR)
INC_FLAGS      := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS)

LDLIBS  := -lm
LDFLAGS :=

# Tools debug is opt-in. Game BUILD_DEBUG / CDEFS must not leak here.
BUILD_DEBUG_TOOLS ?= 0
CDEFS_DEBUG_TOOLS ?= -DSYS_LOG_LEVEL=SYS_LOG_LEVEL_INFO

# Headless: drop graphics flags and log level inherited from game platform makefiles.
override CDEFS := $(filter-out -DSYS_GFX_SOKOL -DSOKOL_GLCORE -DSOKOL_METAL -DSOKOL_D3D11 -DSOKOL_GLES3 -DSOKOL_DEBUG=1 -DSYS_LOG_LEVEL%,$(CDEFS))

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -g
RELEASE_CFLAGS += -DBUILD_DEBUG=0

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DBUILD_DEBUG=1

ifeq ($(BUILD_DEBUG_TOOLS), 1)
	CFLAGS := $(DEBUG_CFLAGS)
	override CDEFS += $(CDEFS_DEBUG_TOOLS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

ASSET_GEN  := $(BUILD_DIR)/luna-asset-gen
ASSET_PACK := $(BUILD_DIR)/luna-asset-pack
META_GEN   := $(BUILD_DIR)/luna-meta-gen

.PHONY: all clean tools tools-meta tools-asset

$(BUILD_DIR):
	mkdir -p "$(BUILD_DIR)"

$(META_GEN): $(SRC_DIR)/meta-gen.c $(LUNA_C_H) $(ROOT_DIR)/tools.mk | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

$(ASSET_GEN): $(SRC_DIR)/asset-gen.c $(LUNA_C_H) $(ROOT_DIR)/tools.mk | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

$(ASSET_PACK): $(SRC_DIR)/asset-pack.c $(LUNA_C_H) $(ROOT_DIR)/tools.mk | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

tools: $(ASSET_GEN) $(ASSET_PACK) $(META_GEN)
tools-meta: tools
tools-asset: tools

clean:
	rm -rf "$(BUILD_DIR)"
