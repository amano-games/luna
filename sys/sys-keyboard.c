#include "sys/sys-keyboard.h"
#include "sys/sys-input.h"

static struct {
	u8 keys[SYS_KEYS_LEN];
} SYS_KEYBOARD;

void
sys_os_keyboard_ini(void)
{
	mclr_array(SYS_KEYBOARD.keys);
}

void
sys_os_keyboard_set(i32 key, b32 down)
{
	if(key >= 0 && key < SYS_KEYS_LEN) {
		SYS_KEYBOARD.keys[key] = down ? 1 : 0;
	}
}

i32
sys_os_keyboard_map(i32 key)
{
	i32 b = 0;

	switch(key) {
	case 'W':
	case SYS_OS_KEY_UP:
		b = SYS_INP_DPAD_U;
		break;
	case 'S':
	case SYS_OS_KEY_DOWN:
		b = SYS_INP_DPAD_D;
		break;
	case 'A':
	case SYS_OS_KEY_LEFT:
		b = SYS_INP_DPAD_L;
		break;
	case 'D':
	case SYS_OS_KEY_RIGHT:
		b = SYS_INP_DPAD_R;
		break;
	case '.':
	case 'X':
	case 'Q':
	case ' ':
		b = SYS_INP_A;
		break;
	case ',':
	case 'Z':
	case 'E':
		b = SYS_INP_B;
		break;
	default:
		break;
	}

	return b;
}

i32
sys_os_keyboard_buttons(void)
{
	i32 b = 0;
	i32 i = 0;

	for(i = 0; i < SYS_KEYS_LEN; i++) {
		if(SYS_KEYBOARD.keys[i] != 0) {
			b |= sys_os_keyboard_map(i);
		}
	}

	return b;
}

int
sys_os_keyboard_get(i32 key)
{
	int res = 0;

	if(key >= 0 && key < SYS_KEYS_LEN) {
		res = SYS_KEYBOARD.keys[key];
	}

	return res;
}

void
sys_os_keyboard_keys(u8 *dest, usize count)
{
	usize n = count;

	if(n > sizeof(SYS_KEYBOARD.keys)) {
		n = sizeof(SYS_KEYBOARD.keys);
	}

	mcpy(dest, SYS_KEYBOARD.keys, n);
}
