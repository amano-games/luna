CC                := gcc
LINUX_INC_DIRS    := $(shell find $(SRC_DIR) -type d)
LINUX_INC_DIRS    += $(shell find $(LUNA_DIR) -type d)
LINUX_INC_FLAGS   += $(addprefix -I,$(LINUX_INC_DIRS))
LINUX_INC_FLAGS   += $(EXTERNAL_FLAGS)

LINUX_TARGET       := $(TARGET).bin
LINUX_LIBS         := -ldl -lm -lrt
LINUX_DEFS         := -std=gnu11 -DBACKEND_SOKOL=1 -DSOKOL_DEBUG=1 -DSOKOL_GLCORE33
LINUX_BUILD_DIR    := $(BUILD_DIR)/linux
LINUX_OBJS         := $(LINUX_BUILD_DIR)/$(LINUX_TARGET)
LINUX_ASSETS_OUT   := $(LINUX_BUILD_DIR)/assets
LINUX_RELEASE_OBJS := $(LINUX_BUILD_DIR)/$(LINUX_TARGET).zip
LINUX_RPATH        := '-Wl,-z,origin -Wl,-rpath,$$ORIGIN/steam-runtime/amd64/lib/x86_64-linux-gnu:$$ORIGIN/steam-runtime/amd64/lib:$$ORIGIN/steam-runtime/amd64/usr/lib/x86_64-linux-gnu:$$ORIGIN/steam-runtime/amd64/usr/lib'

ifeq ($(DETECTED_OS), Linux)
	LINUX_LIBS += -lGL -lX11 -lasound -lXi -lXcursor -lpthread
endif
ifeq ($(DETECTED_OS), Darwin)
	LINUX_LIBS += -fobjc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz
endif

.PHONY: linux linux_run linux_build linux_assets

$(LINUX_ASSETS_OUT): $(ASSETS_BIN) $(WATCH_ASSETS)
	mkdir -p $(LINUX_ASSETS_OUT)
	$(ASSETS_BIN) $(ASSETS_DIR) $(LINUX_ASSETS_OUT)

$(LINUX_BUILD_DIR):
	mkdir -p $(LINUX_BUILD_DIR)
	./update_runtime.sh
	./extract_runtime.sh steam-runtime-release_latest.tar.xz amd64 $(LINUX_BUILD_DIR)/steam-runtime
	cp ./linux/* $(LINUX_BUILD_DIR)

$(LINUX_OBJS): sokol_shader $(LINUX_BUILD_DIR) $(LINUX_ASSETS_OUT) $(WATCH_SRC)
	$(CC) \
		$(LINUX_DEFS) \
		$(LINUX_INC_FLAGS) \
		-finstrument-functions \
		$(CFLAGS) \
		$(WARN) \
		$(SRC_MAIN) \
		$(LINUX_LIBS) \
		-o $(LINUX_OBJS)

$(LINUX_RELEASE_OBJS):
	cd $(LINUX_BUILD_DIR) && zip -r ./$(LINUX_TARGET).zip ./*

linux_clean:
	rm -rf $(LINUX_BUILD_DIR)

linux_run: linux_build
	$(LINUX_BUILD_DIR)/$(TARGET).sh

linux_build: $(LINUX_OBJS)
linux_zip: $(LINUX_RELEASE_OBJS)
linux_assets: $(LINUX_ASSETS_OUT)
linux_release: linux_clean linux_build linux_zip

linux: linux_run

linux_publish: $(LINUX_RELEASE_OBJS)
	butler push $(LINUX_RELEASE_OBJS) amanogames/devils-on-the-moon:linux

