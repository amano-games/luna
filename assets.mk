# Shared asset packing. Platforms set ASSETS_OUT (and optionally
# ASSETS_EXTRA for dirs that must exist first) before including this file.

$(ASSETS_BIN):
	$(MAKE) -f $(LUNA_DIR)/tools.mk tools-asset

.PHONY: assets
assets: $(ASSETS_BIN) $(ASSETS_EXTRA)
	mkdir -p "$(ASSETS_OUT)"
	$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT)
