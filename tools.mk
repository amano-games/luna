.POSIX:
.SUFFIXES:
.PHONY: all clean

ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

SRC_DIR   := $(ROOT_DIR)/tools
DESTDIR   ?=
PREFIX    ?=
BINDIR    ?= ${PREFIX}bin
BUILD_DIR := ${DESTDIR}${BINDIR}

LDLIBS := -lm
LDFLAGS :=

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11

DEBUG_CFLAGS := -std=gnu11 -g3 -O0
DEBUG_CFLAGS += -Werror -Wall -Wextra -pedantic-errors
DEBUG_CFLAGS += -Wdouble-promotion
DEBUG_CFLAGS += -Wno-unused-function
DEBUG_CFLAGS += -Wno-unused-but-set-variable
DEBUG_CFLAGS += -Wno-unused-variable
DEBUG_CFLAGS += -Wno-unused-parameter
DEBUG_CFLAGS += -fsanitize=undefined -fsanitize-trap

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

all: $(BUILD_DIR) $(BUILD_DIR)/luna-table-gen $(BUILD_DIR)/luna-assets

# Create tools bin dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/luna-table-gen: $(SRC_DIR)/table-gen.c
	$(CC) $(CFLAGS) "$<" $(LDLIBS) -o "$@"

$(BUILD_DIR)/luna-assets: $(SRC_DIR)/assets.c
	$(CC) $(CFLAGS) "$<" $(LDLIBS) -o "$@"

# Clean tools bin
clean:
	rm -rf $(BUILD_DIR)
