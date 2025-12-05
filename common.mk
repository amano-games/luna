space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

SRC_DIR      ?= src
LUNA_DIR     ?= luna
GAME_NAME    ?= luna-game
COMPANY_NAME ?= amanogames

DETECTED_OS := $(strip $(shell uname -s))

WARN_FLAGS += -Werror -Wall -Wextra -pedantic-errors
WARN_FLAGS += -Wstrict-prototypes
WARN_FLAGS += -Wshadow
WARN_FLAGS += -Wundef
WARN_FLAGS += -Wdouble-promotion
WARN_FLAGS += -Wno-unused-function
WARN_FLAGS += -Wno-unused-but-set-variable
WARN_FLAGS += -Wno-unused-variable
WARN_FLAGS += -Wno-unused-parameter

ASSETS_DIR := $(SRC_DIR)/assets
ASSETS_BIN := bin/luna-asset-gen

ASSETS_WATCH_SRC := $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)

ifeq ($(DETECTED_OS), Linux)
SHADER_BIN   := $(LUNA_DIR)/external/sokol/shdc/linux/sokol-shdc
endif
ifeq ($(DETECTED_OS), Darwin)
SHADER_BIN   := $(LUNA_DIR)/external/sokol/shdc/osx_arm64/sokol-shdc
endif

SHADER_OBJS  := $(LUNA_DIR)/shaders/sokol_shader.h

# ASSETS_SRC := $(LUNA_DIR)/tools/asset-gen.c
# ASSETS_WATCH_SRC  := $(shell find $(LUNA_DIR) -name *.c -or -name *.s -or -name *.h)
#
# $(ASSETS_BIN): $(ASSETS_WATCH_SRC)
# 	make -f $(LUNA_DIR)/tools.mk CC=$(CC) PREFIX= DESTDIR=
#
# $(SHADER_OBJS): $(LUNA_DIR)/shaders/sokol_shader.glsl
# 	$(SHADER_BIN) --input $< --output $@ --slang glsl410:hlsl5:metal_macos:glsl300es
