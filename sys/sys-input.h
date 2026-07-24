#pragma once

#include "base/types.h"

// This comes from the SOKOL SAPP_MAX_KEYCODES var
#define SYS_KEYS_LEN 512

enum {                         // pd_api.h
	SYS_INP_DPAD_L = (1 << 0), // kButtonLeft
	SYS_INP_DPAD_R = (1 << 1), // kButtonRight
	SYS_INP_DPAD_U = (1 << 2), // kButtonUp
	SYS_INP_DPAD_D = (1 << 3), // kButtonDown
	SYS_INP_B      = (1 << 4), // kButtonB
	SYS_INP_A      = (1 << 5), // kButtonA
	// SYS_INP_MENU         = (1 << 6), // Maybe inp menu and lock button
	SYS_INP_MOUSE_LEFT   = (1 << 8),
	SYS_INP_MOUSE_RIGHT  = (1 << 9),
	SYS_INP_MOUSE_MIDDLE = (1 << 10),
};

// @per_os_impl Input

int sys_inp(void);   // bitmask
f32 sys_crank(void); // [0,1]
int sys_crank_docked(void);
int sys_key(int k);
void sys_keys(u8 *dest, usize count);
f32 sys_mouse_x(void);
f32 sys_mouse_y(void);
