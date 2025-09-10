#include "cam-brain.h"
#include "cam.h"

#include "base/types.h"
#include "base/utils.h"

#include "engine/collisions/collisions.h"
#include "base/mathfunc.h"
#include "base/v2.h"

// TODO: remove the automatic cam bran set pos
void
cam_brain_init(
	struct cam_brain *brain,
	struct cam *c,
	i32 tx,
	i32 ty,
	i32 r,
	f32 drag_lerp_speed,
	f32 lerp_speed,
	f32 limits_speed)
{

	brain->c               = c;
	brain->drag_lerp_speed = drag_lerp_speed;
	brain->lerp_speed      = lerp_speed;
	brain->limits_speed    = limits_speed;
	cam_brain_set_pos(brain, tx, ty, r);
}

void
cam_brain_update(struct cam_brain *brain, f32 tx, f32 ty, i32 r, v2 vel, f32 dt)
{
	i32 index_prev = brain->active_index;
	i32 index      = cam_brain_query_circle(brain, tx, ty, r);
	if(index == -1) { index = index_prev; }
	struct cam_data data = brain->areas[index].data;
	struct cam *cam      = brain->c;
	b32 do_change_cam    = index_prev != index;

	if(do_change_cam) {
		f32 dt_inv                = dt / 1;
		struct cam_data data_prev = brain->areas[index_prev].data;

		brain->active_index = index;
		brain->initial      = brain->c->data;
		brain->final        = data;
		brain->data_t       = 0.0f;
		brain->vel_t        = 0.0f;

		// Set the limits or "snapping" to the current camera viewport
		brain->c->data.soft_limits = (struct col_aabb){
			.min = {cam->p.x - CAM_HALF_W, cam->p.y - CAM_HALF_H},
			.max = {cam->p.x + CAM_HALF_W, cam->p.y + CAM_HALF_H},
		};

		{
			// Calculate the initial -> final drag velocity of the camara
			// If the target velocity is greater than the current area velocity set the initial
			// velocity to the target velocity
			//
			// This is needed because when the camera is dragged by a hard margin the camera velocity
			// is the target velocity.
			f32 cam_vel             = brain->initial.drag_vel;
			f32 target_vel          = abs_f32(vel.y) / dt;
			brain->initial.drag_vel = max_f32(cam_vel, target_vel);
			brain->final.drag_vel   = data.drag_vel;
		}
	}

	cam_update(cam, tx, ty, dt);

	struct cam_data initial = brain->initial;
	struct cam_data final   = brain->final;
	struct cam_data current = brain->c->data;

	if(brain->data_t < 1.0f) {
		f32 lerp_speed = brain->drag_lerp_speed;
		// Lerp from the initial data to the final
		brain->data_t = min_f32(brain->data_t + dt * lerp_speed, 1.0);

		cam->data.soft.min = v2_lerp(initial.soft.min, final.soft.min, brain->data_t);
		cam->data.soft.max = v2_lerp(initial.soft.max, final.soft.max, brain->data_t);

		cam->data.hard.min = v2_lerp(initial.hard.min, final.hard.min, brain->data_t);
		cam->data.hard.max = v2_lerp(initial.hard.max, final.hard.max, brain->data_t);
	}
	{
		f32 limits_vel = brain->limits_speed;
		// Move the limits at constant speed towards the new ones
		cam->data.soft_limits.min = v2_move_towards(data.soft_limits.min, final.soft_limits.min, limits_vel * dt, 1.0);
		cam->data.soft_limits.max = v2_move_towards(data.soft_limits.max, final.soft_limits.max, limits_vel * dt, 1.0);
	}

	if(brain->vel_t < 1.0f) {
		f32 lerp_speed     = brain->lerp_speed;
		brain->vel_t       = min_f32(brain->vel_t + dt * lerp_speed, 1.0f);
		cam->data.drag_vel = lerp(initial.drag_vel, final.drag_vel, brain->vel_t);
	}
}

i32
cam_brain_query_circle(struct cam_brain *brain, f32 x, f32 y, f32 r)
{
	i32 res = -1;
	for(usize i = 0; i < ARRLEN(brain->areas); ++i) {
		struct cam_area c_area = brain->areas[i];
		struct col_aabb aabb   = c_area.aabb;
		b32 is_inside_area     = col_circle_to_aabb(
            x,
            y,
            r,
            aabb.min.x,
            aabb.min.y,
            aabb.max.x,
            aabb.max.y);
		if(is_inside_area) {
			res = i;
			break;
		};
	}
	return res;
}

void
cam_brain_set_data(struct cam_brain *brain, usize index)
{
	struct cam_data data = brain->areas[index].data;
	brain->active_index  = index;
	brain->initial       = data;
	brain->c->data       = data;
	brain->final         = data;
	brain->data_t        = 0.0f;
}

void
cam_brain_set_pos(struct cam_brain *brain, i32 x, i32 y, i32 r)
{
	struct cam *c = brain->c;
	cam_set_pos_px(c, x, y);
	i32 index = cam_brain_query_circle(brain, x, y, r);
	cam_brain_set_data(brain, index);
}
