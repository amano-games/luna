#include "dbg-drw.h"

#include "base/v2.h"
#include "engine/gfx/gfx-defs.h"
#include "engine/gfx/gfx-txt.h"
#include "engine/gfx/gfx.h"
#include "lib/tex/tex.h"
#include "sys/sys.h"

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
dbg_drw_txt(f32 x, f32 y, str8 text, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	fnt_mono_draw_str(
		DBG_DRW_STATE.ctx,
		sys_fnt_mono_get(),
		text,
		(i32)(x + DBG_DRW_STATE.offset.x),
		(i32)(y + DBG_DRW_STATE.offset.y),
		0,
		0,
		PRIM_MODE_WHITE);
}

void
dbg_drw_lin(f32 x1, f32 y1, f32 x2, f32 y2, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_lin(
		DBG_DRW_STATE.ctx,
		x1 + DBG_DRW_STATE.offset.x,
		y1 + DBG_DRW_STATE.offset.y,
		x2 + DBG_DRW_STATE.offset.x,
		y2 + DBG_DRW_STATE.offset.y,
		PRIM_MODE_WHITE);
}

void
dbg_drw_cir(f32 x, f32 y, f32 d, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_cir(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		(i32)d,
		PRIM_MODE_WHITE);
}

void
dbg_drw_ellipsis(f32 x, f32 y, f32 rx, f32 ry, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_ellipsis(
		DBG_DRW_STATE.ctx,
		(i32)(x + DBG_DRW_STATE.offset.x),
		(i32)(y + DBG_DRW_STATE.offset.y),
		(i32)rx,
		(i32)ry,
		PRIM_MODE_WHITE);
}

void
dbg_drw_poly(struct v2 *verts, ssize count, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
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
dbg_drw_tri(f32 ax, f32 ay, f32 bx, f32 by, f32 cx, f32 cy, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
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
dbg_drw_cir_fill(f32 x, f32 y, f32 d, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_cir(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		(i32)d,
		PRIM_MODE_WHITE);
}

void
dbg_drw_rec_i32(struct rec_i32 r, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_rec(
		DBG_DRW_STATE.ctx,
		r.x + DBG_DRW_STATE.offset.x,
		r.y + DBG_DRW_STATE.offset.y,
		r.w,
		r.h,
		PRIM_MODE_WHITE);
}

void
dbg_drw_rec(f32 x, f32 y, f32 w, f32 h, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_rec(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		w,
		h,
		PRIM_MODE_WHITE);
}

void
dbg_drw_rec_fill(f32 x, f32 y, f32 w, f32 h, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_rec_fill(
		DBG_DRW_STATE.ctx,
		x + DBG_DRW_STATE.offset.x,
		y + DBG_DRW_STATE.offset.y,
		w,
		h,
		PRIM_MODE_WHITE);
}

void
dbg_drw_aabb(f32 x1, f32 y1, f32 x2, f32 y2, u8 col)
{
	DBG_DRW_STATE.ctx.color_map[GFX_COL_WHITE] = col;
	gfx_rec(
		DBG_DRW_STATE.ctx,
		x1 + DBG_DRW_STATE.offset.x,
		y1 + DBG_DRW_STATE.offset.y,
		x2 - x1,
		y2 - y1,
		PRIM_MODE_WHITE);
}

void
dbg_drw_collider(struct col_shape shape, u8 col)
{
	switch(shape.type) {
	case COL_TYPE_AABB: {
		struct col_aabb collider = shape.aabb;
		dbg_drw_aabb(collider.min.x, collider.min.y, collider.max.x, collider.max.y, col);
	} break;
	case COL_TYPE_CIR: {
		struct col_cir collider = shape.cir;
		dbg_drw_cir(collider.p.x, collider.p.y, collider.r * 2, col);
	} break;
	case COL_TYPE_CAPSULE: {
		struct col_capsule collider = shape.capsule;
		v2 a                        = collider.a.p;
		f32 ra                      = collider.a.r;
		v2 b                        = collider.b.p;
		f32 rb                      = collider.b.r;

		dbg_drw_cir(a.x, a.y, ra * 2, col);
		dbg_drw_cir(b.x, b.y, rb * 2, col);
		dbg_drw_lin(a.x, a.y, b.x, b.y, col);
		dbg_drw_lin(collider.tangents.a.a.x, collider.tangents.a.a.y, collider.tangents.a.b.x, collider.tangents.a.b.y, col);
		dbg_drw_lin(collider.tangents.b.a.x, collider.tangents.b.a.y, collider.tangents.b.b.x, collider.tangents.b.b.y, col);

	} break;
	case COL_TYPE_POLY: {
		col_poly collider = shape.poly;
		v2 verts[COL_MAX_POLYGON_VERTS];
		for(ssize i = 0; i < collider.count; ++i) {
			verts[i] = col_v2_from_c2v(collider.verts[i]);
		}
		dbg_drw_poly(verts, collider.count, col);
	} break;
	default: {
	} break;
	}
}

#endif
