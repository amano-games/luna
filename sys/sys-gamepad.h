#pragma once

#include "base/types.h"

enum sys_os_gamepad_ev {
	SYS_OS_PAD_EV_NONE,

	SYS_OS_PAD_EV_START,
	SYS_OS_PAD_EV_BACK,
	SYS_OS_PAD_EV_DPAD_U,
	SYS_OS_PAD_EV_DPAD_D,
	SYS_OS_PAD_EV_DPAD_L,
	SYS_OS_PAD_EV_DPAD_R,
	SYS_OS_PAD_EV_A,
	SYS_OS_PAD_EV_B,

	SYS_OS_PAD_EV_NUM_COUNT,
};

void sys_os_gamepad_ini(void);
void sys_os_gamepad_poll(void);
i32 sys_os_gamepad_buttons(void);
b32 sys_os_gamepad_event(enum sys_os_gamepad_ev *ev);
