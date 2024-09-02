# Assets tools
CC             := gcc
TOOLS_DIR      := bin
TOOLS_SRC_DIR  := tools
TOOLS_FLAGS    := -std=gnu11

ASSETS_TARGET   := luna-assets
ASSETS_MAIN     := $(TOOLS_SRC_DIR)/src/assets.c
ASSETS_SRC      := $(ASSETS_MAIN)
ASSETS_BIN      := $(TOOLS_DIR)/$(ASSETS_TARGET)

TABLE_GEN_TARGET   := luna-table-gen
TABLE_GEN_MAIN     := $(TOOLS_SRC_DIR)/src/table-gen.c
TABLE_GEN_SRC      := $(TABLE_GEN_MAIN)
TABLE_GEN_BIN      := $(TOOLS_DIR)/$(TABLE_GEN_TARGET)

.PHONY: assets_bin table_gen_bin tools_bin

# Create tools bin dir
$(TOOLS_DIR):
	mkdir -p $(TOOLS_DIR)

# Clean tools bin
tools_clean:
	rm -rf $(TOOLS_DIR)

# Compile assets bin
$(ASSETS_BIN): $(ASSETS_SRC) | $(TOOLS_DIR)
	$(CC) $(TOOLS_FLAGS) -g3 $(WARN) $(ASSETS_SRC) -o $(ASSETS_BIN) -lm

# Compile table gen bin
$(TABLE_GEN_BIN): $(TABLE_GEN_SRC) | $(TOOLS_DIR)
	$(CC) $(TOOLS_FLAGS) -g3 $(WARN) $(TABLE_GEN_SRC) -o $(TABLE_GEN_BIN) -lm

assets_bin: $(ASSETS_BIN)
table_gen_bin: $(TABLE_GEN_BIN)
tools_bin: tools_clean assets_bin table_gen_bin
