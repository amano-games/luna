#pragma once

#include "cam.h"
#include "sys-types.h"

#define MAX_CAM_AREAS 20

struct cam_brain {
	struct cam *c;
	usize active_index;
	f32 speed_scale;
	f32 drag_lerp_speed;
	f32 limits_speed;
	f32 lerp_speed;
	f32 data_t;
	f32 vel_t;
	struct cam_data initial;
	struct cam_data final;
	struct cam_area areas[MAX_CAM_AREAS];
};

void cam_brain_init(struct cam_brain *brain, struct cam *c, int tx, int ty, int r, f32 drag_lerp_speed, f32 lerp_speed, f32 limits_speed, f32 speed_scale);
void cam_brain_update(struct cam_brain *brain, f32 tx, f32 ty, int r, v2 vel, f32 dt);
int cam_brain_query_circle(struct cam_brain *brain, f32 x, f32 y, f32 r);
void cam_brain_set_data(struct cam_brain *brain, usize index);
