#pragma once

#include "collisions.h"
#include "sys.h"
#include "sys-types.h"

#define CAM_W      SYS_DISPLAY_W
#define CAM_H      SYS_DISPLAY_H
#define CAM_HALF_W (SYS_DISPLAY_W >> 1)
#define CAM_HALF_H (SYS_DISPLAY_H >> 1)

struct cam_data {
	u32 id;
	f32 drag_vel;

	struct col_aabb limits;
	struct col_aabb soft;
	struct col_aabb hard;
};

struct cam {
	v2 p;
	v2 p_final;
	v2 p_initial;
	f32 p_t;
	v2 offs_shake;
	v2 offs;

	struct cam_data data;

	int shake_ticks;
	int shake_ticks_max;
	int shake_str;
};

struct cam_area {
	struct col_aabb aabb;
	struct cam_data data;
};

void cam_screen_shake(struct cam *c, int ticks, int str);
rec_i32 cam_rec_px(struct cam *c);
void cam_set_pos_px(struct cam *c, int x, int y);
void cam_update(struct cam *c, int tx, int ty, f32 dt);
