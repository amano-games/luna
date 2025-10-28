#pragma once

#include "cam.h"
#include "base/types.h"

struct cam_brain {
	struct cam *cam;
	u32 data_id;
	u32 data_id_prev;
	f32 drag_lerp_spd;
	f32 limits_lerp_spd;
	f32 vel_lerp_speed;
	f32 data_t;
	f32 vel_t;
	struct cam_data initial;
	struct cam_data final;
};

void cam_brain_upd(struct cam_brain *brain, f32 tx, f32 ty, f32 dt);
void cam_brain_data_set(struct cam_brain *brain, struct cam_data data, b32 do_lerp);
