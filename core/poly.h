#pragma once

#include "sys-assert.h"
#include "sys-types.h"
#include "sys-utils.h"

#define POLY_MAX_VERTS 8

// https://github.com/schteppe/poly-decomp.js/blob/master/src/index.js

struct poly {
	usize count;
	v2 verts[POLY_MAX_VERTS];
	v2 norms[POLY_MAX_VERTS];
};

// Get the area of a triangle spanned by the three given points. Note that the area will be negative if the points are not given in counter-clockwise order.
static inline i32
poly_triangle_area(v2 a, v2 b, v2 c)
{
	return (((b.x - a.x) * (c.y - a.y)) - ((c.x - a.x) * (b.y - a.y)));
}

static inline bool32
poly_is_left(v2 a, v2 b, v2 c)
{
	return poly_triangle_area(a, b, c) > 0;
}

// Get a vertex at position i. It does not matter if i is out of bounds, this function will just cycle.
static inline v2
poly_at(struct poly *poly, i32 i)
{
	usize s     = poly->count;
	usize index = i < 0 ? i % s + s : i % s;
	return poly->verts[index];
}

// Reverse the vertices in the polygon
static inline void
poly_reverse(struct poly *poly)
{
	if(!poly || poly->count == 0) return;

	usize left  = 0;
	usize right = poly->count - 1;
	while(left < right) {
		v2 temp            = poly->verts[left];
		poly->verts[left]  = poly->verts[right];
		poly->verts[right] = temp;
		left++;
		right--;
	}
}

// TODO: Finish this
static inline bool32
poly_line_segment_intersect(v2 p1, v2 p2, v2 q1, v2 q2)
{
	return false;
}

bool32
poly_make_ccw(struct poly *poly)
{
	assert(poly->count < ARRLEN(poly->verts));
	usize br = 0;
	for(usize i = 0; i < (usize)poly->count; ++i) {
		struct v2 *v = poly->verts;
		if(v[i].y < v[br].y || (v[i].y == v[br].y && v[i].x > v[br].x)) {
			br = i;
		}
	}

	if(!poly_is_left(poly_at(poly, br - 1), poly_at(poly, br), poly_at(poly, br + 1))) {
		poly_reverse(poly);
		return true;
	} else {
		return false;
	}
}

// TODO: finish this
bool32
poly_is_simple(struct poly *poly)
{
	for(usize i = 0; i < poly->count; ++i) {
		for(usize j = 0; j < i - 1; ++j) {
			if(poly_line_segment_intersect(
				   poly->verts[i],
				   poly->verts[i + 1],
				   poly->verts[j],
				   poly->verts[j + 1])) {}
		}
	}
	return false;
}

// Decomposes the polygon into one or more convex sub-Polygons.
static inline void
poly_decomp(struct poly *poly)
{
	// var edges = polygonGetCutEdges(polygon);
	// if(edges.length > 0) {
	// 	return polygonSlice(polygon, edges);
	// } else {
	// 	return [polygon];
	// }
}

v2
poly_centroid(v2 *verts, usize verts_count)
{
	v2 res          = {0};
	f32 signed_area = 0.0;
	f32 x0          = 0.0; // Current vertex X
	f32 y0          = 0.0; // Current vertex Y
	f32 x1          = 0.0; // Next vertex X
	f32 y1          = 0.0; // Next vertex Y
	f32 a           = 0.0; // Partial signed area

	int lastdex = verts_count - 1;
	v2 *prev    = &(verts[lastdex]);
	v2 *next;

	// For all vertices in a loop
	for(usize i = 0; i < verts_count; ++i) {
		next = &(verts[i]);
		x0   = prev->x;
		y0   = prev->y;
		x1   = next->x;
		y1   = next->y;
		a    = x0 * y1 - x1 * y0;
		signed_area += a;
		res.x += (x0 + x1) * a;
		res.y += (y0 + y1) * a;
		prev = next;
	}

	signed_area *= 0.5f;
	res.x /= (6.0f * signed_area);
	res.y /= (6.0f * signed_area);
	return res;
}
