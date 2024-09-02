# While it is usually safe to assume that sensible values have been set for CC and LDD, it does no harm to set them if and only if they are not already set in the environment, using the operator ?=.
SRC_DIR   := ./tools/src
DESTDIR   ?=
PREFIX    ?= ./
BINDIR    ?= ${PREFIX}bin
BUILD_DIR := ${DESTDIR}${BINDIR}

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O3

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

LDLIBS := -lm

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/luna-table-gen $(BUILD_DIR)/luna-assets

# Create tools bin dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/luna-table-gen: $(SRC_DIR)/table-gen.c
	$(CC) $(CFLAGS) $(SRC_DIR)/table-gen.c $(LDLIBS) -o $(BUILD_DIR)/luna-table-gen

$(BUILD_DIR)/luna-assets: $(SRC_DIR)/assets.c
	$(CC) $(CFLAGS) $(SRC_DIR)/table-gen.c $(LDLIBS) -o $(BUILD_DIR)/luna-assets

# Clean tools bin
clean:
	rm -rf $(BUILD_DIR)

