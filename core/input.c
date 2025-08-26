#include "input.h"
#include "dbg.h"
#include "str.h"
#include "sys-utils.h"
#include "trace.h"

static struct inp INP;

void
inp_upd(void)
{
	TRACE_START(__func__);
	INP.prev = INP.curr;
	mcpy(INP.prev.keys, INP.curr.keys, sizeof(INP.curr.keys));
	INP.curr.btn          = sys_inp();
	INP.curr.crank        = sys_crank();
	INP.curr.crank_docked = sys_crank_docked();
	INP.curr.mouse_x      = sys_mouse_x();
	INP.curr.mouse_y      = sys_mouse_y();
	sys_keys(INP.curr.keys, ARRLEN(INP.curr.keys));
	TRACE_END();
}

void
inp_set_buttons(int value)
{
	INP.curr.btn = value;
}

int
inp_get_buttons(void)
{
	return INP.curr.btn;
}

void
inp_set_crank(f32 value)
{
	INP.curr.crank = value;
}

void
inp_set_crank_docked(int value)
{
	INP.curr.crank_docked = value;
}

void
inp_set_keys(u8 *keys, usize count)
{
	dbg_assert(count < ARRLEN(INP.curr.keys));
	mcpy(INP.curr.keys, keys, count);
}

b32
inp_pressed(int b)
{
	return (INP.curr.btn & b);
}

b32
inp_pressed_any(int b)
{
	return (INP.curr.btn & b) != 0;
}

b32
inp_pressed_except(int b)
{
	return (INP.curr.btn & ~b) != 0;
}

b32
inp_pressed_all(int b)
{
	return (INP.curr.btn & b) == b;
}

b32
inp_was_pressed(int b)
{
	return (INP.prev.btn & b);
}

b32
inp_was_pressed_any(int b)
{
	return (INP.prev.btn & b) != 0;
}

b32
inp_was_pressed_all(int b)
{
	return (INP.prev.btn & b) == b;
}

b32
inp_just_pressed(int b)
{
	return inp_pressed(b) && !inp_was_pressed(b);
}

b32
inp_just_released(int b)
{
	b32 res = !inp_pressed(b) && inp_was_pressed(b);
	return res;
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

b32
inp_key_pressed(int key)
{
	i32 upper = char_to_upper(key);
	return INP.curr.keys[upper] == 1;
}

b32
inp_key_was_pressed(int key)
{
	i32 upper = char_to_upper(key);
	return INP.prev.keys[upper] == 1;
}

b32
inp_key_just_pressed(int key)
{
	i32 upper = char_to_upper(key);
	return inp_key_pressed(upper) && !inp_key_was_pressed(upper);
}

b32
inp_key_just_released(int key)
{
	i32 upper = char_to_upper(key);
	b32 res   = !inp_key_pressed(upper) && inp_key_was_pressed(upper);
	return res;
}

f32
inp_mouse_x(void)
{
	f32 res = INP.curr.mouse_x;
	return res;
}

f32
inp_mouse_y(void)
{
	f32 res = INP.curr.mouse_y;
	return res;
}
