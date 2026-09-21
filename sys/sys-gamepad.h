#pragma once

#include "base/types.h"

// Host-private OS gamepad. Not the game-facing sys_inp contract.
// Linux/Windows: minigamepad in sys-gamepad.c.
// macOS/WASM: sys-gamepad-stub.c.

enum sys_os_gamepad_ev {
	SYS_OS_PAD_EV_NONE,

	SYS_OS_PAD_EV_START,
	SYS_OS_PAD_EV_BACK,
	SYS_OS_PAD_EV_DPAD_U,
	SYS_OS_PAD_EV_DPAD_D,
	SYS_OS_PAD_EV_A,
	SYS_OS_PAD_EV_B,

	SYS_OS_PAD_EV_NUM_COUNT,
};

void sys_os_gamepad_ini(void);
void sys_os_gamepad_poll(void);
i32 sys_os_gamepad_buttons(void);
b32 sys_os_gamepad_menu(void);
b32 sys_os_gamepad_event(enum sys_os_gamepad_ev *ev);
