#include "cam-brain.h"
#include "base/dbg.h"
#include "cam.h"

#include "base/types.h"

#include "engine/collisions/collisions.h"
#include "base/mathfunc.h"
#include "base/v2.h"

void
cam_brain_upd(struct cam_brain *brain, f32 tx, f32 ty, f32 dt)
{
	dbg_assert(brain->cam != NULL);
	struct cam *cam = brain->cam;

	cam_upd(cam, tx, ty, dt);

	struct cam_data initial = brain->initial;
	struct cam_data final   = brain->final;
	struct cam_data current = brain->cam->data;

	if(brain->data_t > 0.0f) {
		f32 drag_lerp_spd = brain->drag_lerp_spd;
		// Lerp from the initial data to the final
		brain->data_t = max_f32(brain->data_t - (dt * drag_lerp_spd), 0.0f);

		f32 t              = 1.0f - brain->data_t;
		cam->data.soft.min = v2_lerp(initial.soft.min, final.soft.min, t);
		cam->data.soft.max = v2_lerp(initial.soft.max, final.soft.max, t);

		cam->data.hard.min = v2_lerp(initial.hard.min, final.hard.min, t);
		cam->data.hard.max = v2_lerp(initial.hard.max, final.hard.max, t);
	}
	{
		f32 limits_lerp_spd = brain->limits_lerp_spd;
		// Move the limits at constant speed towards the new ones
		cam->data.soft_limits.min = v2_move_towards(current.soft_limits.min, final.soft_limits.min, limits_lerp_spd * dt, 1.0);
		cam->data.soft_limits.max = v2_move_towards(current.soft_limits.max, final.soft_limits.max, limits_lerp_spd * dt, 1.0);
	}

	if(brain->vel_t < 1.0f) {
		f32 vel_lerp_spd   = brain->vel_lerp_speed;
		brain->vel_t       = min_f32(brain->vel_t + dt * vel_lerp_spd, 1.0f);
		cam->data.drag_vel = lerp(initial.drag_vel, final.drag_vel, brain->vel_t);
	}
}

void
cam_brain_data_set(struct cam_brain *brain, struct cam_data value, b32 do_lerp)
{
	dbg_assert(brain->cam != NULL);
	struct cam *cam     = brain->cam;
	brain->data_id_prev = brain->data_id;
	brain->data_id      = value.id;
	brain->initial      = cam->data;
	brain->final        = value;
	brain->cam->data    = do_lerp ? brain->initial : brain->final;
	brain->data_t       = do_lerp ? 1.0f : 0.0f;

	if(do_lerp) {
		if(brain->data_id_prev != brain->data_id) {
			cam_brain_data_set(brain, value, true);
			// Set the limits or "snapping" to the current camera viewport
			brain->cam->data.soft_limits = (struct col_aabb){
				.min = {cam->p.x - CAM_HALF_W, cam->p.y - CAM_HALF_H},
				.max = {cam->p.x + CAM_HALF_W, cam->p.y + CAM_HALF_H},
			};

#if 0
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
				brain->final.drag_vel   = value.drag_vel;
			}
#endif
		}
	}
}
