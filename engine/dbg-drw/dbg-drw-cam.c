#include "dbg-drw-cam.h"
#include "dbg-drw.h"

void
dbg_drw_cam(struct cam *c)
{
	v2 tp   = c->p_final;
	v2 half = {CAM_HALF_W, CAM_HALF_H};

	struct col_aabb soft = {
		.min = {
			.x = half.x * c->data.soft.min.x,
			.y = half.y * c->data.soft.min.y,
		},
		.max = {
			.x = half.x * c->data.soft.max.x,
			.y = half.y * c->data.soft.max.y,
		},
	};

	struct col_aabb hard = {
		.min = {
			.x = half.x * c->data.hard.min.x,
			.y = half.y * c->data.hard.min.y,
		},
		.max = {
			.x = half.x * c->data.hard.max.x,
			.y = half.y * c->data.hard.max.y,
		},
	};
	struct col_aabb limits = c->data.soft_limits;

	v2_i32 og_offset = dbg_drw_offset_get();

	dgb_drw_offset_set(0, 0);

	// Cross hair
	dbg_drw_lin(half.x - 5, half.y, half.x + 5, half.y);
	dbg_drw_lin(half.x, half.y - 5, half.x, half.y + 5);

	usize dash_size = 10;

	{
		// Drag top
		for(usize i = 0; i < CAM_W / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(i * dash_size, half.y - soft.min.y, i * dash_size + dash_size, half.y - soft.min.y);
			}
		}

		// Drag top hard
		if(hard.min.y != 0) {
			dbg_drw_lin(0, half.y - hard.min.y, SYS_DISPLAY_W, half.y - hard.min.y);
		}
	}

	{
		// Drag left
		for(usize i = 0; i < CAM_H / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(half.x - soft.min.x, i * dash_size, half.x - soft.min.x, i * dash_size + dash_size);
			}
		}

		// Drag right hard
		if(hard.min.x != 0) {
			dbg_drw_lin(half.x - hard.min.x, 0, half.x - hard.min.x, CAM_H);
		}
	}

	{
		// Drag bottom
		for(usize i = 0; i < CAM_W / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(i * dash_size, half.y + soft.max.y, i * dash_size + dash_size, half.y + soft.max.y);
			}
		}

		// Drag bottom hard
		if(hard.max.y != 0) {
			dbg_drw_lin(0, half.y + hard.max.y, CAM_W, half.y + hard.max.y);
		}
	}

	{
		// Drag right
		for(usize i = 0; i < CAM_H / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(half.x + soft.max.x, i * dash_size, half.x + soft.max.x, i * dash_size + dash_size);
			}
		}

		// Drag right hard
		if(hard.max.x != 0) {
			dbg_drw_lin(half.x + hard.max.x, 0, half.x + hard.max.x, CAM_H);
		}
	}

	dgb_drw_offset_set(og_offset.x, og_offset.y);

	dbg_drw_cir(tp.x, tp.y, 2);
}
