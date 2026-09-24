#include "dbg-drw.h"

#include "base/v2.h"
#include "engine/gfx/gfx.h"
#include "lib/tex/tex.h"

#if BUILD_DEBUG && !PD_DEVICE

struct dbg_drw {
	struct gfx_ctx ctx;
	v2_i32 offset;
};

static struct dbg_drw DBG_DRW_STATE;

void
dbg_drw_ctx_set(struct gfx_ctx ctx)
{
	DBG_DRW_STATE.ctx = ctx;
}

v2_i32
dbg_drw_offset_get(void)
{
	return DBG_DRW_STATE.offset;
}

v2_i32
dbg_drw_offset_set(i32 x, i32 y)
{
	v2_i32 res             = DBG_DRW_STATE.offset;
	DBG_DRW_STATE.offset.x = x;
	DBG_DRW_STATE.offset.y = y;
	return res;
}

void
dbg_drw_clr(void)
{
	tex_clr(DBG_DRW_STATE.ctx.dst, GFX_COL_BLACK);
}

void
dbg_drw_lin(f32 x1, f32 y1, f32 x2, f32 y2)
{
	gfx_lin(
		DBG_DRW_STATE.ctx,
		x1 + DBG_DRW_STATE.offset.x,
		y1 + DBG_DRW_STATE.offset.y,
		x2 + DBG_DRW_STATE.offset.x,
		y2 + DBG_DRW_STATE.offset.y,
		PRIM_MODE_WHITE);
}

void
dbg_drw_cir(f32 x, f32 y, f32 d)
{
	gfx_cir(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		(i32)d,
		PRIM_MODE_WHITE);
}

void
dbg_drw_ellipsis(f32 x, f32 y, f32 rx, f32 ry)
{
	gfx_ellipsis(
		DBG_DRW_STATE.ctx,
		(i32)(x + DBG_DRW_STATE.offset.x),
		(i32)(y + DBG_DRW_STATE.offset.y),
		(i32)rx,
		(i32)ry,
		PRIM_MODE_WHITE);
}

void
dbg_drw_poly(struct v2 *verts, ssize count)
{
	for(ssize i = 0; i < count; ++i) {
		v2 a = verts[i];
		v2 b = verts[(i + 1) % count];
		gfx_lin(
			DBG_DRW_STATE.ctx,
			a.x + DBG_DRW_STATE.offset.x,
			a.y + DBG_DRW_STATE.offset.y,
			b.x + DBG_DRW_STATE.offset.x,
			b.y + DBG_DRW_STATE.offset.y,
			PRIM_MODE_WHITE);
	}
}

void
dbg_drw_tri(f32 ax, f32 ay, f32 bx, f32 by, f32 cx, f32 cy)
{
	gfx_tri(
		DBG_DRW_STATE.ctx,
		ax + DBG_DRW_STATE.offset.x,
		ay + DBG_DRW_STATE.offset.y,
		bx + DBG_DRW_STATE.offset.x,
		by + DBG_DRW_STATE.offset.y,
		cx + DBG_DRW_STATE.offset.x,
		cy + DBG_DRW_STATE.offset.y,
		1,
		PRIM_MODE_WHITE);
}

void
dbg_drw_cir_fill(f32 x, f32 y, f32 d)
{
	gfx_cir(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		(i32)d,
		PRIM_MODE_WHITE);
}

void
dbg_drw_rec_i32(struct rec_i32 r)
{
	gfx_rec(
		DBG_DRW_STATE.ctx,
		r.x + DBG_DRW_STATE.offset.x,
		r.y + DBG_DRW_STATE.offset.y,
		r.w,
		r.h,
		PRIM_MODE_WHITE);
}

void
dbg_drw_rec(f32 x, f32 y, f32 w, f32 h)
{
	gfx_rec(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		w,
		h,
		PRIM_MODE_WHITE);
}

void
dbg_drw_rec_fill(f32 x, f32 y, f32 w, f32 h)
{
	gfx_rec_fill(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		w,
		h,
		PRIM_MODE_WHITE);
}

void
dbg_drw_aabb(f32 x1, f32 y1, f32 x2, f32 y2)
{
	gfx_rec(
		DBG_DRW_STATE.ctx,
		x1 + DBG_DRW_STATE.offset.x,
		y1 + DBG_DRW_STATE.offset.y,
		x2 - x1,
		y2 - y1,
		PRIM_MODE_WHITE);
}

// TODO: Re-do all of this
void
dbg_drw_collider(struct col_shape shape)
{
	switch(shape.type) {
	case COL_TYPE_AABB: {
		struct col_aabb col = shape.aabb;
		dbg_drw_aabb(col.min.x, col.min.y, col.max.x, col.max.y);
	} break;
	case COL_TYPE_CIR: {
		struct col_cir col = shape.cir;
		dbg_drw_cir(col.p.x, col.p.y, col.r * 2);
	} break;
	case COL_TYPE_CAPSULE: {
		struct col_capsule col = shape.capsule;
		v2 a                   = col.a.p;
		f32 ra                 = col.a.r;
		v2 b                   = col.b.p;
		f32 rb                 = col.b.r;

		dbg_drw_cir(a.x, a.y, ra * 2);
		dbg_drw_cir(b.x, b.y, rb * 2);
		dbg_drw_lin(a.x, a.y, b.x, b.y);
		dbg_drw_lin(col.tangents.a.a.x, col.tangents.a.a.y, col.tangents.a.b.x, col.tangents.a.b.y);
		dbg_drw_lin(col.tangents.b.a.x, col.tangents.b.a.y, col.tangents.b.b.x, col.tangents.b.b.y);

	} break;
	case COL_TYPE_POLY: {
		col_poly col = shape.poly;
		v2 verts[COL_MAX_POLYGON_VERTS];
		for(ssize i = 0; i < col.count; ++i) {
			verts[i] = col_v2_from_c2v(col.verts[i]);
		}
		dbg_drw_poly(verts, col.count);
	} break;
	default: {
	} break;
	}
}

#endif
