# Shared two-TU game compile helpers.
# Platforms set: CC, CFLAGS, INC_FLAGS, OBJ_DIR, then include this file
# after defining those.

LUNA_SRC ?= $(LUNA_DIR)/luna.c
GAME_SRC ?= $(SRC_DIR)/main.c

DEPFLAGS ?= -MMD -MP

LUNA_OBJ ?= $(OBJ_DIR)/luna.o
GAME_OBJ ?= $(OBJ_DIR)/game.o
UNITY_OBJS ?= $(LUNA_OBJ) $(GAME_OBJ)

$(OBJ_DIR):
	mkdir -p "$(OBJ_DIR)"

$(LUNA_OBJ): $(LUNA_SRC) $(SHADER_OBJS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) $(DEPFLAGS) -c "$<" -o "$@"

$(GAME_OBJ): $(GAME_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) $(DEPFLAGS) -c "$<" -o "$@"

# Pull in compiler-generated header deps (-MMD). Leading '-' ignores missing
# .d files on the first build / after clean.
-include $(LUNA_OBJ:.o=.d)
-include $(GAME_OBJ:.o=.d)
