# Shared asset pipeline: gen → pack → install assets.pck only.
# Platforms may set ASSETS_INSTALL (final .pck path) and ASSETS_EXTRA
# (dirs that must exist first) before including this file.
# Requires BUILD_DIR.

ASSETS_GEN_OUT   ?= $(BUILD_DIR)/assets
ASSETS_PCK       ?= $(BUILD_DIR)/assets.pck
ASSETS_INSTALL   ?= $(ASSETS_PCK)
ASSETS_PALETTE   ?=
ASSETS_ARGS      := --compress $(if $(strip $(ASSETS_PALETTE)),--palette="$(ASSETS_PALETTE)")

ASSETS_SRC := $(shell find "$(ASSETS_DIR)" -not -name '*.tiled-session' | sed 's/ /\\ /g')

HOSTCC ?= clang

$(ASSETS_BIN): $(LUNA_C_H)
	$(MAKE) -f "$(LUNA_DIR)/tools.mk" tools BUILD_DEBUG_TOOLS=0 CC="$(HOSTCC)"

$(ASSETS_PACK_BIN): $(ASSETS_BIN)
	@:

.PHONY: assets

$(ASSETS_INSTALL): $(ASSETS_SRC) $(ASSETS_BIN) $(ASSETS_PACK_BIN) $(ASSETS_PALETTE) | $(ASSETS_EXTRA)
	rm -rf "$(ASSETS_GEN_OUT)"
	mkdir -p "$(ASSETS_GEN_OUT)"
	"$(ASSETS_BIN)" $(ASSETS_ARGS) "$(ASSETS_DIR)" "$(ASSETS_GEN_OUT)"
	mkdir -p "$(dir $(ASSETS_PCK))" "$(dir $(ASSETS_INSTALL))"
	"$(ASSETS_PACK_BIN)" --prefix=assets "$(ASSETS_GEN_OUT)" "$(ASSETS_PCK).tmp"
	mv -f "$(ASSETS_PCK).tmp" "$(ASSETS_INSTALL)"

# Ship only the pack into the platform location.
assets: $(ASSETS_INSTALL)
