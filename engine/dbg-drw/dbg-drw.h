#pragma once

#include "engine/collisions/collisions.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "engine/gfx/gfx.h"

#if BUILD_DEBUG && !PD_DEVICE
void dbg_drw_ctx_set(struct gfx_ctx ctx);
v2_i32 dbg_drw_offset_set(i32 x, i32 y);
v2_i32 dbg_drw_offset_get(void);
void dbg_drw_clr(void);
// Drawing colors are palette indices on 8-bit targets; 1-bit targets use white.
void dbg_drw_txt(f32 x, f32 y, str8 text, u8 col);

void dbg_drw_lin(f32 x1, f32 y1, f32 x2, f32 y2, u8 col);
void dbg_drw_cir(f32 x, f32 y, f32 r, u8 col);
void dbg_drw_cir_fill(f32 x, f32 y, f32 r, u8 col);
void dbg_drw_ellipsis(f32 x, f32 y, f32 rx, f32 ry, u8 col);
void dbg_drw_rec(f32 x, f32 y, f32 w, f32 h, u8 col);
void dbg_drw_rec_fill(f32 x, f32 y, f32 w, f32 h, u8 col);
void dbg_drw_aabb(f32 x1, f32 y1, f32 x2, f32 y2, u8 col);
void dbg_drw_collider(struct col_shape shape, u8 col);
void dbg_drw_rec_i32(struct rec_i32 r, u8 col);
void dbg_drw_poly(struct v2 *verts, ssize count, u8 col);
void dbg_drw_tri(f32 xa, f32 ya, f32 xb, f32 yb, f32 xc, f32 yc, u8 col);

#else

static inline v2_i32
dbg_drw_offset_zero(void)
{ return (v2_i32){0}; }

#define dbg_drw_ctx_set(...)    ((void)0)
#define dbg_drw_offset_set(...) dbg_drw_offset_zero()
#define dbg_drw_offset_get(...) dbg_drw_offset_zero()
#define dbg_drw_clr(...)        ((void)0)
#define dbg_drw_txt(...)        ((void)0)
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
