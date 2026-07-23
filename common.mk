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
WARN_FLAGS += -Wstack-usage=8192
WARN_FLAGS += -Walloca-larger-than=8192

# Daily builds default to debug; pass DEBUG=0 for release.
DEBUG ?= 1

ASSETS_DIR := $(SRC_DIR)/assets
ASSETS_BIN := bin/luna-asset-gen

ifeq ($(DETECTED_OS), Linux)
SHADER_BIN   := $(LUNA_DIR)/external/sokol/shdc/linux/sokol-shdc
endif
ifeq ($(DETECTED_OS), Darwin)
SHADER_BIN   := $(LUNA_DIR)/external/sokol/shdc/osx_arm64/sokol-shdc
endif

SHADER_OBJS  := $(LUNA_DIR)/shaders/sokol_shader.h

