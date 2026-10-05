# Keep relative so recursive $(MAKE) -f stays free of spaces in MAKEFILE_LIST.
ROOT_DIR := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

SRC_DIR      ?= src
LUNA_DIR     ?= luna
GAME_NAME    ?= luna-game
COMPANY_NAME ?= amanogames

DETECTED_OS := $(strip $(shell uname -s))

STEAMWORKS_SDK_PATH ?= $(HOME)/Developer/steamworks-sdk

STEAMWORKS_SDK_PUBLIC    := $(STEAMWORKS_SDK_PATH)/public
STEAMWORKS_SDK_REDIS_BIN := $(STEAMWORKS_SDK_PATH)/redistributable_bin

STEAMWORKS_SDK_LIBDIR_LINUX64 := $(STEAMWORKS_SDK_REDIS_BIN)/linux64
STEAMWORKS_SDK_LIBDIR_WIN64   := $(STEAMWORKS_SDK_REDIS_BIN)/win64
STEAMWORKS_SDK_LIBDIR_OSX     := $(STEAMWORKS_SDK_REDIS_BIN)/osx

STEAMWORKS_SDK_LIB_LINUX64 := $(STEAMWORKS_SDK_LIBDIR_LINUX64)/libsteam_api.so
STEAMWORKS_SDK_LIB_WIN64   := $(STEAMWORKS_SDK_LIBDIR_WIN64)/steam_api64.dll
STEAMWORKS_SDK_LIB_OSX     := $(STEAMWORKS_SDK_LIBDIR_OSX)/libsteam_api.dylib

WARN_FLAGS += -Werror -Wall -Wextra -pedantic-errors
WARN_FLAGS += -Wstrict-prototypes
WARN_FLAGS += -Wshadow
WARN_FLAGS += -Wundef
WARN_FLAGS += -Wdouble-promotion
WARN_FLAGS += -Wmissing-field-initializers
WARN_FLAGS += -Wno-unused-function
WARN_FLAGS += -Wno-unused-but-set-variable
WARN_FLAGS += -Wno-unused-variable
WARN_FLAGS += -Wno-unused-parameter
# WARN_FLAGS += -Wstack-usage=8192
# WARN_FLAGS += -Walloca-larger-than=8192

# Daily builds default to debug; pass BUILD_DEBUG=0 for release.
BUILD_DEBUG ?= 1

ASSETS_DIR      := $(SRC_DIR)/assets
ASSETS_BIN      := bin/luna-asset-gen
ASSETS_PACK_BIN := bin/luna-asset-pack

# Let Make rebuild tools when any luna source/header changes.
LUNA_C_H := $(shell find "$(LUNA_DIR)" -path '*/.git' -prune -o \( -name '*.c' -o -name '*.h' \) -print)

ifeq ($(DETECTED_OS), Linux)
SHADER_BIN   ?= $(LUNA_DIR)/external/sokol/shdc/linux/sokol-shdc
endif
ifeq ($(DETECTED_OS), Darwin)
SHADER_BIN   ?= $(LUNA_DIR)/external/sokol/shdc/osx_arm64/sokol-shdc
endif

SHADER_OBJS  := $(LUNA_DIR)/shaders/sokol_shader.h

.PHONY: shaders
shaders: $(SHADER_OBJS)

$(SHADER_OBJS): $(LUNA_DIR)/shaders/sokol_shader.glsl $(SHADER_BIN) $(LUNA_DIR)/common.mk
	"$(SHADER_BIN)" --input "$<" --output "$@" --slang glsl410:hlsl5:metal_macos:glsl300es

# Let the including Makefile select its default target.
.DEFAULT_GOAL :=
