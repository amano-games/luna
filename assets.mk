# Shared asset pipeline: gen → pack → install assets.pck only.
# Platforms may set ASSETS_INSTALL (final .pck path) and ASSETS_EXTRA
# (dirs that must exist first) before including this file.
# Requires BUILD_DIR.

ASSETS_GEN_OUT   ?= $(BUILD_DIR)/gen-assets/assets
ASSETS_PACK_ROOT ?= $(BUILD_DIR)/gen-assets
ASSETS_PCK       ?= $(BUILD_DIR)/assets.pck
ASSETS_INSTALL   ?= $(ASSETS_PCK)

HOSTCC ?= cc

$(ASSETS_BIN) $(ASSETS_PACK_BIN): $(LUNA_C_H)
	$(MAKE) -f "$(LUNA_DIR)/tools.mk" tools BUILD_DEBUG_TOOLS=0 CC="$(HOSTCC)"

.PHONY: assets_gen assets_pack assets

# Cook src/assets into gen-assets/assets/ so pack members keep the assets/ prefix.
assets_gen: $(ASSETS_BIN) $(ASSETS_EXTRA)
	mkdir -p "$(ASSETS_GEN_OUT)"
	"$(ASSETS_BIN)" --compress "$(ASSETS_DIR)" "$(ASSETS_GEN_OUT)"

assets_pack: assets_gen $(ASSETS_PACK_BIN)
	mkdir -p "$(dir $(ASSETS_PCK))"
	"$(ASSETS_PACK_BIN)" "$(ASSETS_PACK_ROOT)" "$(ASSETS_PCK)"

# Ship only the pack into the platform location.
assets: assets_pack
ifneq ($(abspath $(ASSETS_INSTALL)),$(abspath $(ASSETS_PCK)))
	mkdir -p "$(dir $(ASSETS_INSTALL))"
	cp -f "$(ASSETS_PCK)" "$(ASSETS_INSTALL)"
endif
