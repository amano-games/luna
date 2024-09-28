#include "input.h"
#include "trace.h"

static struct inp INP;

void
inp_update(void)
{
	TRACE_START(__func__);
	INP.prev = INP.curr;
	mcpy(INP.prev.keys, INP.curr.keys, sizeof(INP.curr.keys));
	INP.curr.btn          = sys_inp();
	INP.curr.crank        = sys_crank();
	INP.curr.crank_docked = sys_crank_docked();
	sys_keys(INP.curr.keys);
	TRACE_END();
}

bool32
inp_pressed(int b)
{
	return (INP.curr.btn & b);
}

bool32
inp_pressed_any(int b)
{
	return (INP.curr.btn & b) != 0;
}

bool32
inp_pressed_all(int b)
{
	return (INP.curr.btn & b) == b;
}

bool32
inp_was_pressed(int b)
{
	return (INP.prev.btn & b);
}

bool32
inp_was_pressed_any(int b)
{
	return (INP.prev.btn & b) != 0;
}

bool32
inp_was_pressed_all(int b)
{
	return (INP.prev.btn & b) == b;
}

bool32
inp_just_pressed(int b)
{
	return inp_pressed(b) && !inp_was_pressed(b);
}

bool32
inp_just_released(int b)
{
	return !inp_pressed(b) && inp_was_pressed(b);
}

int
inp_dpad_x(void)
{
	if(inp_pressed(INP_DPAD_L)) return -1;
	if(inp_pressed(INP_DPAD_R)) return +1;
	return 0;
}

int
inp_dpad_y(void)
{
	if(inp_pressed(INP_DPAD_U)) return -1;
	if(inp_pressed(INP_DPAD_D)) return +1;
	return 0;
}

int
inp_dpad_dir(void)
{
	int x = inp_dpad_x();
	int y = inp_dpad_y();
	int d = ((y + 1) << 2) | (x + 1);

	switch(d) {
	case 1: return INP_DPAD_DIR_N;
	case 9: return INP_DPAD_DIR_S;
	case 6: return INP_DPAD_DIR_E;
	case 4: return INP_DPAD_DIR_W;
	case 10: return INP_DPAD_DIR_SE;
	case 8: return INP_DPAD_DIR_SW;
	case 0: return INP_DPAD_DIR_NW;
	case 2: return INP_DPAD_DIR_NE;
	}

	return INP_DPAD_DIR_NONE;
}

f32
inp_crank(void)
{
	return INP.curr.crank;
}

f32
inp_prev_crank(void)
{
	return INP.prev.crank;
}

f32
inp_crank_dt(void)
{
	return inp_crank_calc_dt(INP.prev.crank, INP.curr.crank);
}

f32
inp_crank_calc_dt(f32 ang_from, f32 ang_to)
{
	f32 dt = ang_to - ang_from;
	if(dt <= -0.5f) return (dt + 1.0f);
	if(dt >= 0.5f) return (dt - 1.0f);
	return dt;
}

int
inp_crank_docked(void)
{
	return INP.curr.crank_docked;
}

int
inp_crank_was_docked(void)
{
	return INP.prev.crank_docked;
}

int
inp_crank_just_docked(void)
{
	return inp_crank_docked() && !inp_crank_was_docked();
}

int
inp_crank_just_undocked(void)
{
	return !inp_crank_docked() && inp_crank_was_docked();
}

bool32
inp_key_pressed(int key)
{
	return INP.curr.keys[key] == 1;
}

bool32
inp_key_was_pressed(int key)
{
	return INP.prev.keys[key] == 1;
}

bool32
inp_key_just_pressed(int key)
{
	return inp_key_pressed(key) && !inp_key_was_pressed(key);
}

bool32
inp_key_just_released(int key)
{
	return !inp_key_pressed(key) && inp_key_was_pressed(key);
}
