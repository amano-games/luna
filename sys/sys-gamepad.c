#include "sys/sys-gamepad.h"
#include "sys/sys-input.h"

#include "base/utils.h"

/* Vendored minigamepad has known UB in mapping parse; keep sanitizers off for it. */
#if defined(__clang__)
#pragma clang attribute push( \
	__attribute__((no_sanitize("address", "undefined", "unreachable"))), \
	apply_to = function)
#elif defined(__GNUC__)
#define MG_API __attribute__((no_sanitize("address", "undefined", "unreachable")))
#endif
#define MG_IMPLEMENTATION
#include "minigamepad.h"
#if defined(__clang__)
#pragma clang attribute pop
#endif

#define SYS_GAMEPAD_AXIS_DEADZONE 0.8f

static struct {
	mg_gamepads pads;
	i32 buttons;
	b32 menu;
} SYS_GAMEPAD;

// First connected pad → Playdate-style SYS_INP_* bits.
//   South / West / R1 / RT → A
//   East  / North / L1 / LT → B
//   D-pad + left stick     → SYS_INP_DPAD_*
static i32
sys_gamepad_map_pad(const mg_gamepad *pad)
{
	i32 res = 0;
	ssize i = 0;

	for(i = 0; i < MG_BUTTON_COUNT; i++) {
		mg_button_state button = pad->buttons[i];
		if(button.supported == MG_FALSE || button.current == MG_FALSE) {
			continue;
		}
		switch((mg_button)i) {
		case MG_BUTTON_SOUTH:
		case MG_BUTTON_WEST:
		case MG_BUTTON_RIGHT_SHOULDER:
			res |= SYS_INP_A;
			break;
		case MG_BUTTON_EAST:
		case MG_BUTTON_NORTH:
		case MG_BUTTON_LEFT_SHOULDER:
			res |= SYS_INP_B;
			break;
		case MG_BUTTON_DPAD_LEFT:
			res |= SYS_INP_DPAD_L;
			break;
		case MG_BUTTON_DPAD_RIGHT:
			res |= SYS_INP_DPAD_R;
			break;
		case MG_BUTTON_DPAD_UP:
			res |= SYS_INP_DPAD_U;
			break;
		case MG_BUTTON_DPAD_DOWN:
			res |= SYS_INP_DPAD_D;
			break;
		default:
			break;
		}
	}

	for(i = 0; i < MG_AXIS_COUNT; i++) {
		f32 value = pad->axes[i].value;
		switch((mg_axis)i) {
		case MG_AXIS_LEFT_X:
			if(value > SYS_GAMEPAD_AXIS_DEADZONE) {
				res |= SYS_INP_DPAD_R;
			}
			if(value < -SYS_GAMEPAD_AXIS_DEADZONE) {
				res |= SYS_INP_DPAD_L;
			}
			break;
		case MG_AXIS_LEFT_Y:
			if(value > SYS_GAMEPAD_AXIS_DEADZONE) {
				res |= SYS_INP_DPAD_D;
			}
			if(value < -SYS_GAMEPAD_AXIS_DEADZONE) {
				res |= SYS_INP_DPAD_U;
			}
			break;
		case MG_AXIS_LEFT_TRIGGER:
			if(value > SYS_GAMEPAD_AXIS_DEADZONE) {
				res |= SYS_INP_B;
			}
			break;
		case MG_AXIS_RIGHT_TRIGGER:
			if(value > SYS_GAMEPAD_AXIS_DEADZONE) {
				res |= SYS_INP_A;
			}
			break;
		default:
			break;
		}
	}

	return res;
}

static b32
sys_gamepad_map_menu(const mg_gamepad *pad)
{
	mg_button btn[] = {
		MG_BUTTON_START,
		MG_BUTTON_BACK,
		MG_BUTTON_GUIDE,
		MG_BUTTON_MISC1,
	};
	b32 held = false;
	i32 n    = 0;

	for(n = 0; n < (i32)ARRLEN(btn); n++) {
		mg_button_state s = pad->buttons[btn[n]];
		if(s.supported == MG_TRUE && s.current == MG_TRUE) {
			held = true;
		}
	}

	return held;
}

void
sys_os_gamepad_ini(void)
{
	mg_gamepads_init(&SYS_GAMEPAD.pads);
	// Queue on so poll fills events for Sokol pause; button state still updates if full.
	SYS_GAMEPAD.pads.queue_events = MG_TRUE;
	SYS_GAMEPAD.buttons           = 0;
	SYS_GAMEPAD.menu              = false;
}

void
sys_os_gamepad_poll(void)
{
	mg_gamepad *pad = NULL;

	SYS_GAMEPAD.pads.queue_events = MG_TRUE;
	mg_gamepads_poll(&SYS_GAMEPAD.pads);

	pad                 = SYS_GAMEPAD.pads.list.head;
	SYS_GAMEPAD.buttons = 0;
	SYS_GAMEPAD.menu    = false;
	if(pad != NULL) {
		SYS_GAMEPAD.buttons = sys_gamepad_map_pad(pad);
		SYS_GAMEPAD.menu    = sys_gamepad_map_menu(pad);
	}
}

i32
sys_os_gamepad_buttons(void)
{
	return SYS_GAMEPAD.buttons;
}

b32
sys_os_gamepad_menu(void)
{
	return SYS_GAMEPAD.menu;
}

// Drain one interesting press. Uses the queued snapshot from poll, not another poll.
b32
sys_os_gamepad_event(enum sys_os_gamepad_ev *ev)
{
	b32 got     = false;
	mg_event mg = {0};

	*ev = SYS_OS_PAD_EV_NONE;

	while(got == false) {
		if(mg_gamepads_check_queued_event(&SYS_GAMEPAD.pads, &mg) == MG_FALSE) {
			break;
		}
		if(mg.type != MG_EVENT_BUTTON_PRESS) {
			continue;
		}
		switch(mg.button) {
		case MG_BUTTON_START: {
			*ev = SYS_OS_PAD_EV_START;
			got = true;
		} break;
		case MG_BUTTON_BACK: {
			*ev = SYS_OS_PAD_EV_BACK;
			got = true;
		} break;
		case MG_BUTTON_DPAD_UP: {
			*ev = SYS_OS_PAD_EV_DPAD_U;
			got = true;
		} break;
		case MG_BUTTON_DPAD_DOWN: {
			*ev = SYS_OS_PAD_EV_DPAD_D;
			got = true;
		} break;
		case MG_BUTTON_SOUTH: {
			*ev = SYS_OS_PAD_EV_A;
			got = true;
		} break;
		case MG_BUTTON_EAST: {
			*ev = SYS_OS_PAD_EV_B;
			got = true;
		} break;
		default: {
		} break;
		}
	}

	return got;
}
