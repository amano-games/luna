#pragma once

#include "engine/collisions/collisions.h"
#include "base/mem.h"
#include "base/types.h"
#include "engine/gfx/gfx.h"

#if BUILD_DEBUG && !PD_DEVICE
void dbg_drw_ctx_set(struct gfx_ctx ctx);
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

#else

static inline v2_i32
dbg_drw_offset_zero(void)
{ return (v2_i32){0}; }

#define dbg_drw_ctx_set(...)    ((void)0)
#define dbg_drw_offset_set(...) dbg_drw_offset_zero()
#define dbg_drw_offset_get(...) dbg_drw_offset_zero()
#define dbg_drw_clr(...)        ((void)0)
#define dbg_drw_lin(...)        ((void)0)
#define dbg_drw_cir(...)        ((void)0)
#define dbg_drw_cir_fill(...)   ((void)0)
#define dbg_drw_ellipsis(...)   ((void)0)
#define dbg_drw_rec(...)        ((void)0)
#define dbg_drw_rec_fill(...)   ((void)0)
#define dbg_drw_aabb(...)       ((void)0)
#define dbg_drw_collider(...)   ((void)0)
#define dbg_drw_rec_i32(...)    ((void)0)
#define dbg_drw_poly(...)       ((void)0)
#define dbg_drw_tri(...)        ((void)0)
#endif
