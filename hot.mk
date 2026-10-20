# Hot code reloading
# Platforms set GAME_TMP_PREFIX, then include this file before using CDEFS
# Later they set HOT_HOST (the host exe), GAME_SO and GAME_LOCK, and include game.mk
# Names are relative to the exe dir and passed to the host as SYS_HOT_* defines
HOT             ?= 0
GAME_SO_NAME    := game.so
GAME_LOCK_NAME  := game.lock

ifeq ($(HOT),1)
ifeq ($(BUILD_DEBUG),0)
$(error HOT=1 is debug only)
endif
override CDEFS := $(CDEFS) \
	-DSYS_HOT_RELOAD=1 \
	-DSYS_HOT_LIB_PATH=\"$(GAME_SO_NAME)\" \
	-DSYS_HOT_LOCK_PATH=\"$(GAME_LOCK_NAME)\" \
	-DSYS_HOT_TMP_PREFIX=\"$(GAME_TMP_PREFIX)\"
endif

# HOT=1 rebuilds hold game.lock until done
# Sub-makes get GAME_LOCK_HELD=1 so only the outermost make takes the lock
GAME_LOCK_HELD  ?= 0
GAME_LOCK_BEGIN  =
GAME_LOCK_END    =

ifeq ($(HOT),1)
ifeq ($(GAME_LOCK_HELD),0)
GAME_LOCK_BEGIN  = mkdir -p "$(dir $(GAME_LOCK))"; touch "$(GAME_LOCK)";
GAME_LOCK_END    = ; rc=$$?; rm -f "$(GAME_LOCK)"; exit $$rc
endif
endif

ifeq ($(HOT),1)
HOST_OBJS     = $(LUNA_OBJ)
BUILD_TARGETS = $(HOT_HOST) $(GAME_SO)
else
HOST_OBJS     = $(UNITY_OBJS)
BUILD_TARGETS = $(HOT_HOST)
endif
