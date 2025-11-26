#pragma once

#include "engine/collisions/collisions.h"
#include "base/mem.h"
#include "base/types.h"

struct dbg_drw {
	v2_i32 drw_offset;
	struct debug_shape *shapes;
};

void dbg_drw_ini(struct alloc alloc, ssize shapes_count);
void dbg_drw(i32 x, i32 y);
v2_i32 dbg_drw_offset_set(i32 x, i32 y);
v2_i32 dbg_drw_offset_get(void);
void dbg_drw_clr(void);

void dbg_drw_lin(f32 x1, f32 y1, f32 x2, f32 y2);
void dbg_drw_cir(f32 x, f32 y, f32 r);
void dbg_drw_cir_fill(f32 x, f32 y, f32 r);
void dbg_drw_ellipsis(f32 x, f32 y, f32 rx, f32 ry);
void dbg_drw_rec(f32 x, f32 y, f32 w, f32 h);
void dbg_drw_rec_fill(f32 x, f32 y, f32 w, f32 h);
void dbg_drw_aabb(f32 x1, f32 y1, f32 x2, f32 y2);
void dbg_drw_collider(struct col_shape shape);
void dbg_drw_rec_i32(struct rec_i32 r);
void dbg_drw_poly(struct v2 *verts, ssize count);
void dbg_drw_tri(f32 xa, f32 ya, f32 xb, f32 yb, f32 xc, f32 yc);

void dgb_drw_shape_push(struct debug_shape shape);
