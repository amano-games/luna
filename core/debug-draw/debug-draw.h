#pragma once

#include "collisions.h"
#include "layout.h"
#include "mem-arena.h"
#include "mem.h"
#include "sys-types.h"

struct debug_draw DEBUG_STATE;

#define MAX_DEBUG_SHAPES 10000

struct debug_draw {
	v2_i32 draw_offset;
	struct debug_shape *shapes;

	struct alloc alloc;
	struct marena marena;
	alignas(8) char *mem;
};

void *debug_mem_alloc(usize s);

void debug_draw_init(void *mem, usize size);
void debug_draw_draw(i32 x, i32 y);
void debug_draw_set_offset(i32 x, i32 y);
v2_i32 debug_draw_get_offset(void);
void debug_draw_clear(void);

void debug_draw_line(f32 x1, f32 y1, f32 x2, f32 y2);
void debug_draw_cir(f32 x, f32 y, f32 r);
void debug_draw_cir_fill(f32 x, f32 y, f32 r);
void debug_draw_rec(f32 x, f32 y, f32 w, f32 h);
void debug_draw_rec_fill(f32 x, f32 y, f32 w, f32 h);
void debug_draw_aabb(f32 x1, f32 y1, f32 x2, f32 y2);
void debug_draw_collider(struct col_shape shape);
void debug_draw_ui_rec(struct ui_rec rec);
void debug_draw_poly(struct v2 *verts, size count);
void debug_draw_tri(f32 xa, f32 ya, f32 xb, f32 yb, f32 xc, f32 yc);

void debug_draw_push_shape(struct debug_shape shape);
