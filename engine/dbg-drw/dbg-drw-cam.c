#include "sys/sys.h"
#include "dbg-drw-cam.h"
#include "dbg-drw.h"
#include "engine/cam/cam.h"
#include "sys/sys-defs.h"

#if BUILD_DEBUG && !PD_DEVICE

void
dbg_drw_cam(struct cam *c, u8 col)
{
	v2_i32 sys_resolution = sys_resolution_get();
	v2 tp   = c->p_final;
	v2_i32 sys_resolution_half = {sys_resolution.x >> 1, sys_resolution.y >> 1};

	struct col_aabb soft = {
		.min = {
			.x = sys_resolution_half.x * c->data.soft_drag.min.x,
			.y = sys_resolution_half.y * c->data.soft_drag.min.y,
		},
		.max = {
			.x = sys_resolution_half.x * c->data.soft_drag.max.x,
			.y = sys_resolution_half.y * c->data.soft_drag.max.y,
		},
	};

	struct col_aabb hard = {
		.min = {
			.x = sys_resolution_half.x * c->data.hard_drag.min.x,
			.y = sys_resolution_half.y * c->data.hard_drag.min.y,
		},
		.max = {
			.x = sys_resolution_half.x * c->data.hard_drag.max.x,
			.y = sys_resolution_half.y * c->data.hard_drag.max.y,
		},
	};
	struct col_aabb limits = c->data.soft_limits;

	v2_i32 og_offset = dbg_drw_offset_get();

	dbg_drw_offset_set(0, 0);

	// Cross hair
	dbg_drw_lin(sys_resolution_half.x - 5, sys_resolution_half.y, sys_resolution_half.x + 5, sys_resolution_half.y, col);
	dbg_drw_lin(sys_resolution_half.x, sys_resolution_half.y - 5, sys_resolution_half.x, sys_resolution_half.y + 5, col);

	usize dash_size = 10;

	{
		// Drag top
		for(usize i = 0; i < sys_resolution.x / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(i * dash_size, sys_resolution_half.y - soft.min.y, i * dash_size + dash_size, sys_resolution_half.y - soft.min.y, col);
			}
		}

		// Drag top hard
		if(hard.min.y != 0) {
			dbg_drw_lin(0, sys_resolution_half.y - hard.min.y, sys_resolution.x, sys_resolution_half.y - hard.min.y, col);
		}
	}

	{
		// Drag left
		for(usize i = 0; i < sys_resolution.y / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(sys_resolution_half.x - soft.min.x, i * dash_size, sys_resolution_half.x - soft.min.x, i * dash_size + dash_size, col);
			}
		}

		// Drag right hard
		if(hard.min.x != 0) {
			dbg_drw_lin(sys_resolution_half.x - hard.min.x, 0, sys_resolution_half.x - hard.min.x, sys_resolution.y, col);
		}
	}

	{
		// Drag bottom
		for(usize i = 0; i < sys_resolution.x / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(i * dash_size, sys_resolution_half.y + soft.max.y, i * dash_size + dash_size, sys_resolution_half.y + soft.max.y, col);
			}
		}

		// Drag bottom hard
		if(hard.max.y != 0) {
			dbg_drw_lin(0, sys_resolution_half.y + hard.max.y, sys_resolution.x, sys_resolution_half.y + hard.max.y, col);
		}
	}

	{
		// Drag right
		for(usize i = 0; i < sys_resolution.y / dash_size; ++i) {
			if(i % 2 == 0) {
				dbg_drw_lin(sys_resolution_half.x + soft.max.x, i * dash_size, sys_resolution_half.x + soft.max.x, i * dash_size + dash_size, col);
			}
		}

		// Drag right hard
		if(hard.max.x != 0) {
			dbg_drw_lin(sys_resolution_half.x + hard.max.x, 0, sys_resolution_half.x + hard.max.x, sys_resolution.y, col);
		}
	}

	dbg_drw_offset_set(og_offset.x, og_offset.y);

	dbg_drw_cir(tp.x, tp.y, 2, col);
}

#endif
