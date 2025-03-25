#include "debug-draw.h"

#include "arr.h"
#include "v2.h"
#include "sys-debug.h"

void
debug_draw_init(void *mem, usize size)
{
	log_info("Debug draw", "init");
	DEBUG_STATE.mem   = mem;
	DEBUG_STATE.alloc = marena_allocator(&DEBUG_STATE.marena);
	marena_init(&DEBUG_STATE.marena, DEBUG_STATE.mem, size);

#if !defined(TARGET_PLAYDATE) && DEBUG
	DEBUG_STATE.shapes = arr_ini(MAX_DEBUG_SHAPES, sizeof(struct debug_shape), DEBUG_STATE.alloc);
	arr_clear(DEBUG_STATE.shapes);
#endif
}

void
debug_draw_draw(v2_i32 cam_off)
{
#if !defined(TARGET_PLAYDATE) && defined(DEBUG)
	TRACE_START(__func__);

	debug_draw_set_offset(cam_off.x, cam_off.y);

	sys_debug_draw(DEBUG_STATE.shapes, arr_len(DEBUG_STATE.shapes));
	debug_draw_clear();
	TRACE_END();
#endif
}

v2_i32
debug_draw_get_offset(void)
{
	return DEBUG_STATE.draw_offset;
}

void
debug_draw_set_offset(i32 x, i32 y)
{
	DEBUG_STATE.draw_offset.x = x;
	DEBUG_STATE.draw_offset.y = y;
}

void
debug_draw_clear(void)
{
	arr_clear(DEBUG_STATE.shapes);
}
void
debug_draw_push_shape(struct debug_shape shape)
{
#if !defined(TARGET_PLAYDATE) && defined(DEBUG)
	arr_push(DEBUG_STATE.shapes, shape);
#endif
}

void
debug_draw_line(f32 x1, f32 y1, f32 x2, f32 y2)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_LIN;

	d_shape.lin.a = v2_add_i32(v2_round((v2){x1, y1}), DEBUG_STATE.draw_offset);
	d_shape.lin.b = v2_add_i32(v2_round((v2){x2, y2}), DEBUG_STATE.draw_offset);

	debug_draw_push_shape(d_shape);
}

void
debug_draw_cir(f32 x, f32 y, f32 d)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_CIR;
	d_shape.cir.p              = v2_add_i32(v2_round((v2){x, y}), DEBUG_STATE.draw_offset);
	d_shape.cir.d              = (int)d;
	debug_draw_push_shape(d_shape);
}

void
debug_draw_cir_fill(f32 x, f32 y, f32 d)
{
	struct debug_shape d_shape = {0};
	d_shape.cir.filled         = true;
	d_shape.type               = DEBUG_CIR;
	d_shape.cir.p              = v2_add_i32(v2_round((v2){x, y}), DEBUG_STATE.draw_offset);
	d_shape.cir.d              = (int)d;
	debug_draw_push_shape(d_shape);
}

void
debug_draw_rec(f32 x, f32 y, f32 w, f32 h)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_REC;

	d_shape.rec.x = x + DEBUG_STATE.draw_offset.x;
	d_shape.rec.y = y + DEBUG_STATE.draw_offset.y;

	d_shape.rec.w = w;
	d_shape.rec.h = h;

	debug_draw_push_shape(d_shape);
}

void
debug_draw_rec_fill(f32 x, f32 y, f32 w, f32 h)
{
	struct debug_shape d_shape = {0};
	d_shape.type               = DEBUG_REC;

	d_shape.rec.x = x + DEBUG_STATE.draw_offset.x;
	d_shape.rec.y = y + DEBUG_STATE.draw_offset.y;

	d_shape.rec.w = w;
	d_shape.rec.h = h;

	d_shape.rec.filled = true;

	debug_draw_push_shape(d_shape);
}

void
debug_draw_aabb(f32 x1, f32 y1, f32 x2, f32 y2)
{
	debug_draw_rec(x1, y1, x2 - x1, y2 - y1);
}

void
debug_draw_ui_rec(struct ui_rec rec)
{
	debug_draw_aabb(rec.minx, rec.miny, rec.maxx, rec.maxy);
}

// TODO: Re-do all of this
void
debug_draw_collider(struct col_shape *shape)
{
	switch(shape->type) {
	case COL_TYPE_AABB: {
		struct col_aabb col = shape->aabb;
		debug_draw_aabb(col.min.x, col.min.y, col.max.x, col.max.y);
	} break;
	case COL_TYPE_CIR: {
		struct col_cir col = shape->cir;
		debug_draw_cir(col.p.x, col.p.y, col.r * 2);
	} break;
	case COL_TYPE_CAPSULE: {
		struct col_capsule col = shape->capsule;
		v2 a                   = col.a;
		f32 ra                 = col.ra;
		v2 b                   = col.b;
		f32 rb                 = col.rb;

		debug_draw_cir(a.x, a.y, ra * 2);
		debug_draw_cir(b.x, b.y, rb * 2);
		debug_draw_line(a.x, a.y, b.x, b.y);
		debug_draw_line(col.tangents.a.a.x, col.tangents.a.a.y, col.tangents.a.b.x, col.tangents.a.b.y);
		debug_draw_line(col.tangents.b.a.x, col.tangents.b.a.y, col.tangents.b.b.x, col.tangents.b.b.y);

	} break;
	case COL_TYPE_POLY: {
		struct col_poly col = shape->poly;

		for(usize i = 0; i < col.count; ++i) {
			struct poly sub_poly = col.sub_polys[i];
			for(usize j = 0; j < sub_poly.count; ++j) {
				v2 a = sub_poly.verts[j];
				v2 b = sub_poly.verts[(j + 1) % sub_poly.count];
				debug_draw_line(a.x, a.y, b.x, b.y);
			}
		}
	} break;
	default: {
	} break;
	}
}
