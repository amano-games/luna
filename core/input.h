#pragma once

#include "sys-input.h"

enum {
	INP_A      = SYS_INP_A,
	INP_B      = SYS_INP_B,
	INP_DPAD_U = SYS_INP_DPAD_U,
	INP_DPAD_D = SYS_INP_DPAD_D,
	INP_DPAD_L = SYS_INP_DPAD_L,
	INP_DPAD_R = SYS_INP_DPAD_R,
};

enum {
	INP_DPAD_DIR_NONE,
	INP_DPAD_DIR_N,
	INP_DPAD_DIR_S,
	INP_DPAD_DIR_E,
	INP_DPAD_DIR_W,
	INP_DPAD_DIR_NE,
	INP_DPAD_DIR_NW,
	INP_DPAD_DIR_SE,
	INP_DPAD_DIR_SW,
};

struct inp_state {
	int btn;
	int crank_docked;
	f32 crank;
	u8 keys[SYS_KEYS_LEN];
};

struct inp {
	struct inp_state curr;
	struct inp_state prev;
};

void inp_update(void);

bool32 inp_pressed(int b);
bool32 inp_pressed_any(int b);
bool32 inp_pressed_all(int b);
bool32 inp_was_pressed(int b);
bool32 inp_was_pressed_any(int b);
bool32 inp_was_pressed_all(int b);
bool32 inp_just_pressed(int b);
bool32 inp_just_released(int b);

bool32 inp_key_pressed(int key);
bool32 inp_key_was_pressed(int key);
bool32 inp_key_just_pressed(int key);
bool32 inp_key_just_released(int key);

int inp_dpad_x(void);
int inp_dpad_y(void);
int inp_dpad_dir(void);
f32 inp_crank(void);
f32 inp_prev_crank(void);
f32 inp_crank_dt(void);
f32 inp_crank_calc_dt(f32 ang_from, f32 ang_to);
int inp_crank_docked(void);
int inp_crank_was_docked(void);
int inp_crank_just_docked(void);
int inp_crank_just_undocked(void);
