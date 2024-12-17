space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

include $(ROOT_DIR)/common.mk

SRC_DIR   := $(ROOT_DIR)/tools
DESTDIR   ?=
PREFIX    ?=
BINDIR    ?= ${PREFIX}bin
BUILD_DIR := ${DESTDIR}${BINDIR}

WATCH_SRC      := $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)
EXTERNAL_DIRS  := $(LUNA_DIR)/external $(LUNA_DIR)/external/sokol
EXTERNAL_FLAGS := $(addprefix -isystem,$(EXTERNAL_DIRS))
INC_DIRS       := $(shell find $(SRC_DIR) -type d)
INC_DIRS       += $(LUNA_DIR) $(LUNA_DIR)/sys $(LUNA_DIR)/lib $(LUNA_DIR)/core
INC_FLAGS      += $(addprefix -I,$(INC_DIRS))
INC_FLAGS      += $(EXTERNAL_FLAGS)

LDLIBS := -lm
LDFLAGS :=

override CDEFS := $(CDEFS) -DBACKEND_CLI

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -g3

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -fsanitize-trap -fsanitize=address,unreachable

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

all: $(BUILD_DIR) $(BUILD_DIR)/luna-meta-gen $(BUILD_DIR)/luna-assets-gen

# Create tools bin dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/luna-meta-gen: $(SRC_DIR)/meta-gen.c $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

$(BUILD_DIR)/luna-asset-gen: $(SRC_DIR)/asset-gen.c $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) "$<" $(LDLIBS) -o "$@"

# Clean tools bin
clean:
	rm -rf $(BUILD_DIR)

luna-meta-gen: $(BUILD_DIR)/luna-meta-gen
luna-assets-gen: $(BUILD_DIR)/luna-asset-gen
