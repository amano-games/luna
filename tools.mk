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

# Headless: drop graphics flags inherited from game platform makefiles.
override CDEFS := $(filter-out -DSYS_GFX_SOKOL -DSOKOL_GLCORE -DSOKOL_METAL -DSOKOL_D3D11 -DSOKOL_GLES3 -DSOKOL_DEBUG=1,$(CDEFS))

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -g

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += $(WARN_FLAGS)

ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

ASSET_GEN := $(BUILD_DIR)/luna-asset-gen
META_GEN  := $(BUILD_DIR)/luna-meta-gen

.PHONY: all clean tools-meta tools-asset

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(META_GEN): $(SRC_DIR)/meta-gen.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

$(ASSET_GEN): $(SRC_DIR)/asset-gen.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

tools-meta: $(META_GEN)
tools-asset: $(ASSET_GEN)

clean:
	rm -rf $(BUILD_DIR)
