#include "cam/cam.h"
#include "mathfunc.h"
#include "rndm.h"
#include "sys-log.h"
#include "v2.h"

rec_i32
cam_rec_px(struct cam *c)
{
	v2_i32 p  = v2_round(v2_add(c->p, c->offset));
	rec_i32 r = {p.x - CAM_HALF_W, p.y - CAM_HALF_H, CAM_W, CAM_H};

	r.x &= ~1; // avoid dither flickering -> snap camera pos
	r.y &= ~1;
	return r;
}

void
cam_set_pos_px(struct cam *c, int x, int y)
{
	c->p.x = (f32)x;
	c->p.y = (f32)y;
}

v2
cam_drag_position(struct cam *c, int tx, int ty, v2 min, v2 max)
{
	v2 target_p = {tx, ty};
	v2 cam_p    = {c->p.x, c->p.y};

	// Convert the margin min / max
	// from a range of 0.0,1.0 to pixels
	f32 left   = min.x * CAM_HALF_W;
	f32 top    = min.y * CAM_HALF_H;
	f32 right  = max.x * CAM_HALF_W;
	f32 bottom = max.y * CAM_HALF_H;

	// Margin Vertical
	f32 min_y = target_p.y - bottom;
	f32 max_y = target_p.y + top;

	// Margin horizontal
	f32 min_x = target_p.x - right;
	f32 max_x = target_p.x + left;

	v2 res = {
		.x = clamp_f32(cam_p.x, min_x, max_x),
		.y = clamp_f32(cam_p.y, min_y, max_y),
	};

	return res;
}

v2
cam_limit_position(v2 p, struct col_aabb limits)
{

	f32 left   = limits.min.x + CAM_HALF_W;
	f32 top    = limits.min.y + CAM_HALF_H;
	f32 right  = limits.max.x - CAM_HALF_W;
	f32 bottom = limits.max.y - CAM_HALF_H;

	v2 v = {
		clamp_f32(p.x, left, right),
		clamp_f32(p.y, top, bottom),
	};

	return v;
}

void
cam_update(struct cam *c, int tx, int ty, f32 dt)
{
	v2 cam_pos                  = c->p;
	struct cam_data data        = c->data;
	struct col_aabb hard_limits = data.hard_limits;
	f32 smoothing_speed         = data.drag_vel;

	v2 soft_pos = cam_drag_position(c, tx, ty, data.soft.min, data.soft.max);
	soft_pos    = cam_limit_position(soft_pos, data.soft_limits);

	v2 a = cam_pos;
	v2 b = soft_pos;

	// apply hard drag margin and if there it modifies the camera position
	// 	set the camera position to that position without lerp
	v2 hard_pos = cam_pos;
	if(
		data.hard.min.x != 0 ||
		data.hard.min.y != 0 ||
		data.hard.max.x != 0 ||
		data.hard.max.y != 0) {
		hard_pos = cam_drag_position(c, tx, ty, data.hard.min, data.hard.max);
	}

	if(hard_pos.y != cam_pos.y || hard_pos.x != cam_pos.x) {
		cam_pos    = hard_pos;
		c->p_final = hard_pos;
	} else {
#if 1
		// Move by a constant speed
		c->p_final = b;
		{
			f32 delta = b.y - a.y;
			f32 sign  = sgn_f32(delta);
			f32 speed = min_f32(abs_f32(delta), dt * smoothing_speed);

			cam_pos.y = a.y + (sign * speed);
		}
		{
			f32 delta = b.x - a.x;
			f32 sign  = sgn_f32(delta);
			f32 speed = min_f32(abs_f32(delta), dt * smoothing_speed);

			cam_pos.x = a.x + (sign * speed);
		}
#else
		// Exponential smoothing
		// https://lisyarus.github.io/blog/posts/exponential-smoothing.html

		cam_pos.y += (b.y - a.y) * (1.0 - exp_f32(-smoothing_speed * dt));

#endif
	}

	v2 limited_pos = cam_limit_position(cam_pos, hard_limits);
	c->p           = limited_pos;

	if(c->shake_ticks) {
		i32 shake_x = (c->shake_str_x * c->shake_ticks + (c->shake_ticks_max >> 1)) / c->shake_ticks_max;
		i32 shake_y = (c->shake_str_y * c->shake_ticks + (c->shake_ticks_max >> 1)) / c->shake_ticks_max;
		c->shake_ticks--;
		c->offset.x = rndm_range_i32(NULL, -shake_x, shake_x);
		c->offset.y = rndm_range_i32(NULL, -shake_y, shake_y);
	}
}

void
cam_shake_xy(struct cam *c, i32 ticks, i32 str_x, i32 str_y)
{
	c->shake_ticks     = ticks;
	c->shake_ticks_max = ticks;
	c->shake_str_x     = str_x;
	c->shake_str_y     = str_y;
}

void
cam_shake(struct cam *c, i32 ticks, i32 str)
{
	cam_shake_xy(c, ticks, str, str);
}
