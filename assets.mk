# Shared asset packing. Platforms set ASSETS_OUT (and optionally
# ASSETS_TIMESTAMP_EXTRA) before including this file.

ASSETS_TIMESTAMP ?= $(ASSETS_OUT)/.timestamp

.PHONY: FORCE
FORCE:

$(ASSETS_BIN): FORCE
	$(MAKE) -f $(LUNA_DIR)/tools.mk tools-asset

# Stamp file: recipe always runs (FORCE) but only repacks when needed.
$(ASSETS_TIMESTAMP): $(ASSETS_BIN) $(ASSETS_TIMESTAMP_EXTRA) FORCE
	mkdir -p "$(ASSETS_OUT)"
	@if [ ! -f "$@" ] || [ "$(ASSETS_BIN)" -nt "$@" ] || \
		find $(ASSETS_DIR) -type f -newer "$@" -print -quit 2>/dev/null | grep -q .; then \
		$(ASSETS_BIN) $(ASSETS_DIR) $(ASSETS_OUT); \
		touch "$@"; \
	fi
