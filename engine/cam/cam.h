#pragma once

#include "engine/collisions/collisions.h"
#include "sys/sys.h"
#include "base/types.h"

#define CAM_W      SYS_DISPLAY_W
#define CAM_H      SYS_DISPLAY_H
#define CAM_HALF_W (SYS_DISPLAY_W >> 1)
#define CAM_HALF_H (SYS_DISPLAY_H >> 1)

struct cam_data {
	u32 id;
	f32 drag_vel;

	struct col_aabb soft_limits;
	struct col_aabb hard_limits;
	struct col_aabb soft;
	struct col_aabb hard;
};

struct cam {
	v2 p;
	v2 p_final;
	v2 p_initial;
	f32 p_t;

	struct cam_data data;

	v2 offset;
	// v2 shake;
	// f32 shake_spd;

	u16 shake_ticks;
	u16 shake_ticks_max;
	u16 shake_str_x;
	u16 shake_str_y;
};

rec_i32 cam_rec_px(struct cam *c);
void cam_set_pos_px(struct cam *c, int x, int y);
void cam_upd(struct cam *c, int tx, int ty, f32 dt);
void cam_shake(struct cam *c, i32 ticks, i32 str);
void cam_shake_xy(struct cam *c, i32 ticks, i32 str_x, i32 str_y);
v2 cam_limit_position(v2 p, struct col_aabb limits);
