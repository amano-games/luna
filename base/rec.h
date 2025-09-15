#pragma once

#include "base/mathfunc.h"
#include "base/types.h"

enum rec_cut_side {
	REC_CUT_NONE,
	REC_CUT_LEFT,
	REC_CUT_RIGHT,
	REC_CUT_TOP,
	REC_CUT_BOTTOM,
};

#define REC_UNPACK(r) r.x, r.y, r.w, r.h

static inline struct rec_i32
rec_i32_inset(struct rec_i32 r, i32 a)
{
	struct rec_i32 res = {
		.x = r.x + a,
		.y = r.y + a,
		.w = max_i32(0, r.w - 2 * a),
		.h = max_i32(0, r.h - 2 * a),
	};
	return res;
}

static inline struct rec_i32
rec_i32_inset_x(struct rec_i32 r, i32 a)
{
	struct rec_i32 res = {
		.x = r.x + a,
		.y = r.y,
		.w = max_i32(0, r.w - 2 * a),
		.h = r.h,
	};
	return res;
}

static inline struct rec_i32
rec_i32_inset_y(struct rec_i32 r, i32 a)
{
	struct rec_i32 res = {
		.x = r.x,
		.y = r.y + a,
		.w = r.w,
		.h = max_i32(0, r.h - 2 * a),
	};
	return res;
}

static inline struct rec_i32
rec_i32_expand(struct rec_i32 r, i32 a)
{
	struct rec_i32 res = {
		.x = r.x - a,
		.y = r.y - a,
		.w = r.w + 2 * a,
		.h = r.h + 2 * a,
	};
	return res;
}

static inline struct rec_i32
rec_i32_expand_x(struct rec_i32 r, i32 a)
{
	struct rec_i32 res = {
		.x = r.x - a,
		.y = r.y,
		.w = r.w + 2 * a,
		.h = r.h,
	};
	return res;
}

static inline struct rec_i32
rec_i32_expand_y(struct rec_i32 r, i32 a)
{
	struct rec_i32 res = {
		.x = r.x,
		.y = r.y - a,
		.w = r.w,
		.h = r.h + 2 * a,
	};
	return res;
}

static inline struct rec_i32
rec_i32_union(struct rec_i32 a, struct rec_i32 b)
{
	i32 x1             = min_i32(a.x, b.x);
	i32 y1             = min_i32(a.y, b.y);
	i32 x2             = max_i32(a.x + a.w, b.x + b.w);
	i32 y2             = max_i32(a.y + a.h, b.y + b.h);
	struct rec_i32 res = {x1, y1, x2 - x1, y2 - y1};
	return res;
}

static inline struct rec_i32
rec_intersection(struct rec_i32 a, struct rec_i32 b)
{
	i32 x1             = max_i32(a.x, b.x);
	i32 y1             = max_i32(a.y, b.y);
	i32 x2             = min_i32(a.x + a.w, b.x + b.w);
	i32 y2             = min_i32(a.y + a.h, b.y + b.h);
	struct rec_i32 res = {
		.x = x1,
		.y = y1,
		.w = max_i32(0, x2 - x1),
		.h = max_i32(0, y2 - y1),
	};
	return res;
}

static inline struct rec_i32
rec_i32_clamp_inside(struct rec_i32 r, struct rec_i32 bounds)
{
	if(r.x < bounds.x) {
		r.w -= (bounds.x - r.x);
		r.x = bounds.x;
	}
	if(r.y < bounds.y) {
		r.h -= (bounds.y - r.y);
		r.y = bounds.y;
	}
	if(r.x + r.w > bounds.x + bounds.w) r.w = bounds.x + bounds.w - r.x;
	if(r.y + r.h > bounds.y + bounds.h) r.h = bounds.y + bounds.h - r.y;
	if(r.w < 0) r.w = 0;
	if(r.h < 0) r.h = 0;
	return r;
}

static inline i32
rec_i32_right(struct rec_i32 r)
{
	return r.x + r.w;
}

static inline i32
rec_i32_bottom(struct rec_i32 r)
{
	return r.y + r.h;
}

static inline v2_i32
rec_i32_cntr(struct rec_i32 r)
{
	v2_i32 res = {.x = r.x + (r.w * 0.5f), .y = r.y + (r.h * 0.5f)};
	return res;
}

static inline struct rec_i32
rec_i32_anchor(struct rec_i32 parent, struct rec_i32 r, struct v2 pivot)
{
	struct rec_i32 res = r;
	res.x              = parent.x + (i32)((parent.w - r.w) * pivot.x);
	res.y              = parent.y + (i32)((parent.h - r.h) * pivot.y);
	return res;
}

struct rec_i32
rec_i32_cut_left(struct rec_i32 *r, i32 a)
{
	i32 cut_w          = min_i32(r->w, a);
	struct rec_i32 res = {r->x, r->y, cut_w, r->h};
	r->x += cut_w;
	r->w -= cut_w;
	return res;
}

struct rec_i32
rec_i32_cut_right(struct rec_i32 *r, i32 a)
{
	i32 cut_w          = min_i32(r->w, a);
	struct rec_i32 res = {r->x + r->w - cut_w, r->y, cut_w, r->h};
	r->w -= cut_w;
	return res;
}

struct rec_i32
rec_i32_cut_top(struct rec_i32 *r, i32 a)
{
	i32 cut_h          = min_i32(r->h, a);
	struct rec_i32 res = {r->x, r->y, r->w, cut_h};
	r->y += cut_h;
	r->h -= cut_h;
	return res;
}

struct rec_i32
rec_i32_cut_bottom(struct rec_i32 *r, i32 a)
{
	i32 cut_h          = min_i32(r->h, a);
	struct rec_i32 res = {r->x, r->y + r->h - cut_h, r->w, cut_h};
	r->h -= cut_h;
	return res;
}

rec_i32
rec_i32_cut(rec_i32 *r, enum rec_cut_side side, i32 a)
{
	switch(side) {
	case REC_CUT_LEFT: return rec_i32_cut_left(r, a);
	case REC_CUT_RIGHT: return rec_i32_cut_right(r, a);
	case REC_CUT_TOP: return rec_i32_cut_top(r, a);
	case REC_CUT_BOTTOM: return rec_i32_cut_bottom(r, a);
	default: {
		dbg_sentinel("rec");
	}
	}

error:
	return (struct rec_i32){0};
}
