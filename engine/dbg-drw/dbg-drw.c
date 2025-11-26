#include "dbg-drw.h"

#include "base/arr.h"
#include "base/v2.h"
#include "sys/sys-debug-draw.h"

struct dbg_drw DBG_DRW_STATE;

void
dbg_drw_ini(struct alloc alloc, ssize shapes_count)
{
	log_info("Debug draw", "init");
	struct dbg_drw *state = &DBG_DRW_STATE;
#if !defined(TARGET_PLAYDATE) && DEBUG && !defined(APP_DISABLE_DEBUG_DRAW)
	state->shapes = arr_new_clr(state->shapes, shapes_count, alloc);
#else
	DBG_DRW_STATE.shapes = NULL;
#endif
}

void
dbg_drw(i32 x, i32 y)
{
#if !defined(TARGET_PLAYDATE) && defined(DEBUG) && !defined(APP_DISABLE_DEBUG_DRAW)
	TRACE_START(__func__);
	dbg_drw_offset_set(x, y);
	sys_debug_draw(DBG_DRW_STATE.shapes, arr_len(DBG_DRW_STATE.shapes));
	dbg_drw_clr();
	TRACE_END();
#endif
}

v2_i32
dbg_drw_offset_get(void)
{
	return DBG_DRW_STATE.drw_offset;
}

v2_i32
dbg_drw_offset_set(i32 x, i32 y)
{
	v2_i32 res                 = DBG_DRW_STATE.drw_offset;
	DBG_DRW_STATE.drw_offset.x = x;
	DBG_DRW_STATE.drw_offset.y = y;
	return res;
}

void
dbg_drw_clr(void)
{
	arr_reset(DBG_DRW_STATE.shapes);
}

void
dgb_drw_shape_push(struct debug_shape shape)
{
#if !defined(TARGET_PLAYDATE) && defined(DEBUG) && !defined(APP_DISABLE_DEBUG_DRAW)
	arr_push(DBG_DRW_STATE.shapes, shape);
#endif
}

void
dbg_drw_lin(f32 x1, f32 y1, f32 x2, f32 y2)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_LIN;

	d_shape.lin.a = v2_add_i32(v2_round((v2){x1, y1}), DBG_DRW_STATE.drw_offset);
	d_shape.lin.b = v2_add_i32(v2_round((v2){x2, y2}), DBG_DRW_STATE.drw_offset);

	dgb_drw_shape_push(d_shape);
}

void
dbg_drw_cir(f32 x, f32 y, f32 d)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_CIR;
	d_shape.cir.p              = v2_add_i32(v2_round((v2){x, y}), DBG_DRW_STATE.drw_offset);
	d_shape.cir.d              = (int)d;
	dgb_drw_shape_push(d_shape);
}

void
dbg_drw_ellipsis(f32 x, f32 y, f32 rx, f32 ry)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_ELLIPSIS;
	d_shape.ellipsis.x         = x + DBG_DRW_STATE.drw_offset.x;
	d_shape.ellipsis.y         = y + DBG_DRW_STATE.drw_offset.y;
	d_shape.ellipsis.rx        = rx;
	d_shape.ellipsis.ry        = ry;
	dgb_drw_shape_push(d_shape);
}

void
dbg_drw_poly(struct v2 *verts, ssize count)
{
	for(ssize i = 0; i < count; ++i) {
		v2 a = verts[i];
		v2 b = verts[(i + 1) % count];
		dbg_drw_lin(a.x, a.y, b.x, b.y);
	}
}

void
dbg_drw_tri(f32 xa, f32 ya, f32 xb, f32 yb, f32 xc, f32 yc)
{
	v2 verts[3] = {
		{xa, ya},
		{xb, yb},
		{xc, yc},
	};
	dbg_drw_poly(verts, 3);
}

void
dbg_drw_cir_fill(f32 x, f32 y, f32 d)
{
	struct debug_shape d_shape = {0};
	d_shape.cir.filled         = true;
	d_shape.type               = DEBUG_CIR;
	d_shape.cir.p              = v2_add_i32(v2_round((v2){x, y}), DBG_DRW_STATE.drw_offset);
	d_shape.cir.d              = (int)d;
	dgb_drw_shape_push(d_shape);
}

void
dbg_drw_rec_i32(struct rec_i32 r)
{
	dbg_drw_rec(r.x, r.y, r.w, r.h);
}

void
dbg_drw_rec(f32 x, f32 y, f32 w, f32 h)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_REC;

	d_shape.rec.x = x + DBG_DRW_STATE.drw_offset.x;
	d_shape.rec.y = y + DBG_DRW_STATE.drw_offset.y;

	d_shape.rec.w = w;
	d_shape.rec.h = h;

	dgb_drw_shape_push(d_shape);
}

void
dbg_drw_rec_fill(f32 x, f32 y, f32 w, f32 h)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_REC;

	d_shape.rec.x = x + DBG_DRW_STATE.drw_offset.x;
	d_shape.rec.y = y + DBG_DRW_STATE.drw_offset.y;

	d_shape.rec.w = w;
	d_shape.rec.h = h;

	d_shape.rec.filled = true;

	dgb_drw_shape_push(d_shape);
}

void
dbg_drw_aabb(f32 x1, f32 y1, f32 x2, f32 y2)
{
	dbg_drw_rec(x1, y1, x2 - x1, y2 - y1);
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
		struct col_poly col = shape.poly;
		dbg_drw_poly(col.verts, col.count);
	} break;
	default: {
	} break;
	}
}
