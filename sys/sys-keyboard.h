#pragma once

#include "base/types.h"

// Host-private OS keyboard. Not the game-facing sys_inp contract.
// Sokol/DRM feed held keys; letters are ASCII. Arrows match Sokol/GLFW.

enum {
	SYS_OS_KEY_RIGHT = 262,
	SYS_OS_KEY_LEFT  = 263,
	SYS_OS_KEY_DOWN  = 264,
	SYS_OS_KEY_UP    = 265,
};

void sys_os_keyboard_ini(void);
void sys_os_keyboard_set(i32 key, b32 down);
i32 sys_os_keyboard_buttons(void);
i32 sys_os_keyboard_map(i32 key);
int sys_os_keyboard_get(i32 key);
void sys_os_keyboard_keys(u8 *dest, usize count);
