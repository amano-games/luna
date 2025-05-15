#pragma once

#include "sys-types.h"
#include "v2.h"

static inline void
tri_barycentric(v2 p, v2 a, v2 b, v2 c, f32 *u, f32 *v, f32 *w)
{
	v2 v0     = v2_sub(b, a);
	v2 v1     = v2_sub(c, a);
	v2 v2     = v2_sub(p, a);
	f32 d00   = v2_dot(v0, v0);
	f32 d01   = v2_dot(v0, v1);
	f32 d11   = v2_dot(v1, v1);
	f32 d20   = v2_dot(v2, v0);
	f32 d21   = v2_dot(v2, v1);
	f32 denom = d00 * d11 - d01 * d01;
	*v        = (d11 * d20 - d01 * d21) / denom;
	*w        = (d00 * d21 - d01 * d20) / denom;
	*u        = 1.0f - *v - *w;
}

bool32
tri_is_point_inside(v2 p, v2 a, v2 b, v2 c)
{
	f32 u = 0;
	f32 v = 0;
	f32 w = 0;
	tri_barycentric(p, a, b, c, &u, &v, &w);

	return v >= 0.0f && w >= 0.0f && (v + w) <= 1.0f;
}

// Returns 2 times the signed triangle area. The result is positive if
// abc is ccw, negative if abc is cw, zero if abc is degenerate.
static inline f32
tri_signed_2d_area(v2 a, v2 b, v2 c)
{
	// (b - a) × (c - a) a as origin
	// f32 res = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	// (a - c) × (b - c) c as origin
	f32 res = (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
	return res;
}

static inline bool32
tri_is_ccw(v2 a, v2 b, v2 c)
{
	f32 area = tri_signed_2d_area(a, b, c);
	return area > 0;
}
