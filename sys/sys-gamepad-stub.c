#include "sys/sys-gamepad.h"

void
sys_os_gamepad_init(void)
{
}

void
sys_os_gamepad_poll(void)
{
}

i32
sys_os_gamepad_buttons(void)
{
	return 0;
}

b32
sys_os_gamepad_menu(void)
{
	return false;
}

b32
sys_os_gamepad_event(enum sys_os_gamepad_ev *ev)
{
	*ev = SYS_OS_PAD_EV_NONE;
	return false;
}
