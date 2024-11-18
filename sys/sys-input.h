#pragma once

#include "sys-types.h"

// This comes from the SOKOL SAPP_MAX_KEYCODES var
#define SYS_KEYS_LEN 512

enum {                     // pd_api.h
	SYS_INP_DPAD_L = 0x01, // kButtonLeft
	SYS_INP_DPAD_R = 0x02, // kButtonRight
	SYS_INP_DPAD_U = 0x04, // kButtonUp
	SYS_INP_DPAD_D = 0x08, // kButtonDown
	SYS_INP_B      = 0x10, // kButtonB
	SYS_INP_A      = 0x20, // kButtonA
};

int sys_inp(void);   // bitmask
f32 sys_crank(void); // [0,1]
int sys_crank_docked(void);
int sys_key(int k);
void sys_keys(u8 *dest, usize count);
