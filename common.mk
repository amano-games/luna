space := $(null) $(null)
ROOT_DIR := $(subst $(space),\ ,$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))"))

SRC_DIR      ?= src
LUNA_DIR     ?= luna
GAME_NAME    ?= luna-game
COMPANY_NAME ?= amanogames

DETECTED_OS := $(shell uname -s)
DETECTED_OS := $(strip $(DETECTED_OS))
$(info DETECTED_OS is "$(DETECTED_OS)")

WARN_FLAGS += -Werror -Wall -Wextra -pedantic-errors
WARN_FLAGS += -Wdouble-promotion
WARN_FLAGS += -Wno-unused-function
WARN_FLAGS += -Wno-unused-but-set-variable
WARN_FLAGS += -Wno-unused-variable
WARN_FLAGS += -Wno-unused-parameter

ASSETS_DIR := $(SRC_DIR)/assets
ASSETS_BIN := bin/luna-assets

SHADER_BIN   := bin/sokol-shdc
SHADER_OBJS  := $(LUNA_DIR)/shaders/sokol_shader.h

clean:
	rm -rf $(DESTDIR)

$(ASSETS_BIN):
	make -f $(LUNA_DIR)/tools.mk CC=$(CC) PREFIX= DESTDIR=

$(SHADER_OBJS): $(LUNA_DIR)/shaders/sokol_shader.glsl
	$(SHADER_BIN) --input $< --output $@ --slang glsl330:hlsl5:metal_macos:glsl300es
