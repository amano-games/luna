space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

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
DEPFLAGS := -MMD -MP

override CDEFS := $(CDEFS) -DBACKEND_CLI

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
	$(CC) $(CFLAGS) $(INC_FLAGS) $(DEPFLAGS) -MF "$@.d" "$<" $(LDLIBS) -o "$@"

$(ASSET_GEN): $(SRC_DIR)/asset-gen.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) $(DEPFLAGS) -MF "$@.d" "$<" $(LDLIBS) -o "$@"

-include $(ASSET_GEN).d
-include $(META_GEN).d

tools-meta: $(META_GEN)
tools-asset: $(ASSET_GEN)

clean:
	rm -rf $(BUILD_DIR)
