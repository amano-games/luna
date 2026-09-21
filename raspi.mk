# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/common.mk

DESTDIR      ?=
PREFIX       ?=
SYSROOT      ?=
PLATFORM_DIR := platforms/raspi
TARGET       := $(GAME_NAME).bin

CC ?= armv6-rpi-linux-gnueabihf-gcc

RASPI_ARCH := -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -marm
RASPI_DISPLAY_SCALE_H := 3
RASPI_PLANE_ROT_DEG := 270

RELEASE_BINDIR := ${PREFIX}raspi-release
ifeq ($(BUILD_DEBUG),0)
BINDIR ?= $(RELEASE_BINDIR)
else
BINDIR ?= ${PREFIX}raspi
endif

BUILD_DIR := ${DESTDIR}${BINDIR}

LDLIBS := -lm -ldrm -lasound -lpthread -lrt -ldl

SYSROOT_CFLAGS :=
SYSROOT_LDFLAGS :=
ifneq ($(SYSROOT),)
# Debian multiarch keeps crt1.o in usr/lib/arm-linux-gnueabihf, not usr/lib.
SYSROOT_B := $(SYSROOT)/usr/lib/arm-linux-gnueabihf
SYSROOT_CFLAGS  += --sysroot=$(SYSROOT) -B$(SYSROOT_B)
SYSROOT_LDFLAGS += --sysroot=$(SYSROOT) -B$(SYSROOT_B)
SYSROOT_LDFLAGS += -L$(SYSROOT)/usr/lib/arm-linux-gnueabihf
SYSROOT_LDFLAGS += -L$(SYSROOT)/lib/arm-linux-gnueabihf
SYSROOT_LDFLAGS += -Wl,-rpath-link,$(SYSROOT)/usr/lib/arm-linux-gnueabihf
SYSROOT_LDFLAGS += -Wl,-rpath-link,$(SYSROOT)/lib/arm-linux-gnueabihf
SYSROOT_INC := -isystem $(SYSROOT)/usr/include/arm-linux-gnueabihf
SYSROOT_INC += -isystem $(SYSROOT)/usr/include
SYSROOT_INC += -isystem $(SYSROOT)/usr/include/libdrm
else
SYSROOT_INC := -isystem /usr/include/libdrm
endif

LDFLAGS += $(SYSROOT_LDFLAGS)

EXTERNAL_DIRS  := $(LUNA_DIR)/external
EXTERNAL_FLAGS := $(EXTERNAL_DIRS:%=-isystem %)

INC_DIRS  := src $(LUNA_DIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(EXTERNAL_FLAGS) $(SYSROOT_INC)

override CDEFS := $(CDEFS) -DSYS_GFX_DRM -D_GNU_SOURCE -DSYS_DISPLAY_SCALE_H=$(RASPI_DISPLAY_SCALE_H) -DDRM_PLANE_ROT_DEG=$(RASPI_PLANE_ROT_DEG)

RELEASE_CFLAGS := ${CFLAGS}
RELEASE_CFLAGS += -std=gnu11 -O2 -g
RELEASE_CFLAGS += -DNDEBUG
RELEASE_CFLAGS += -DBUILD_DEBUG=0
RELEASE_CFLAGS += $(WARN_FLAGS)
RELEASE_CFLAGS += -fno-omit-frame-pointer
RELEASE_CFLAGS += $(RASPI_ARCH)
RELEASE_CFLAGS += -Wno-psabi
RELEASE_CFLAGS += $(SYSROOT_CFLAGS)

DEBUG_CFLAGS := -std=gnu11 -g -O0
DEBUG_CFLAGS += -fno-omit-frame-pointer
DEBUG_CFLAGS += $(WARN_FLAGS)
DEBUG_CFLAGS += -DBUILD_DEBUG=1
DEBUG_CFLAGS += $(RASPI_ARCH)
DEBUG_CFLAGS += -Wno-psabi
DEBUG_CFLAGS += $(SYSROOT_CFLAGS)

ifeq ($(BUILD_DEBUG), 1)
	CFLAGS := $(DEBUG_CFLAGS)
else
	CFLAGS := $(RELEASE_CFLAGS)
endif

CFLAGS += $(CDEFS)

OBJ_DIR := $(BUILD_DIR)/obj
BINARY  := $(BUILD_DIR)/$(TARGET)

include $(ROOT_DIR)/game.mk
include $(ROOT_DIR)/assets.mk

.PHONY: all clean build release
.DEFAULT_GOAL := all

all: build

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cp -fr $(PLATFORM_DIR)/. $(BUILD_DIR)

$(BINARY): $(UNITY_OBJS) | $(BUILD_DIR) assets
	$(CC) $(CFLAGS) $(UNITY_OBJS) $(LDLIBS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

build:
	$(MAKE) -f $(ROOT_DIR)/raspi.mk clean DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) SYSROOT=$(SYSROOT) CC=$(CC)
	$(MAKE) -f $(ROOT_DIR)/raspi.mk $(BINARY) DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) BUILD_DEBUG=$(BUILD_DEBUG) CDEFS="$(CDEFS)" SYSROOT=$(SYSROOT) CC=$(CC)

release:
	$(MAKE) -f $(ROOT_DIR)/raspi.mk build BUILD_DEBUG=0 DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) GAME_NAME=$(GAME_NAME) CDEFS="$(CDEFS)" SYSROOT=$(SYSROOT) CC=$(CC)
