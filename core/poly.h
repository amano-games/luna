#pragma once

#include "sys-assert.h"
#include "sys-types.h"
#include "tri.h"
#include "v2.h"

#define POLY_MAX_VERTS 8

// https://github.com/schteppe/poly-decomp.js/blob/master/src/index.js

struct poly {
	size count;
	v2 verts[POLY_MAX_VERTS];
	v2 norms[POLY_MAX_VERTS];
};

struct tri {
	v2 verts[3];
};

struct mesh {
	size count;
	struct tri *triangles;
};

// Get the area of a triangle spanned by the three given points. Note that the area will be negative if the points are not given in counter-clockwise order.
static inline f32
poly_triangle_area(v2 a, v2 b, v2 c)
{
	return (((b.x - a.x) * (c.y - a.y)) - ((c.x - a.x) * (b.y - a.y)));
}

static inline bool32
poly_is_left(v2 a, v2 b, v2 c)
{
	return poly_triangle_area(a, b, c) > 0;
}

static inline bool32
poly_is_left_on(v2 a, v2 b, v2 c)
{
	return poly_triangle_area(a, b, c) >= 0;
}

static inline bool32
poly_is_rigth(v2 a, v2 b, v2 c)
{
	return poly_triangle_area(a, b, c) < 0;
}

static inline bool32
poly_is_right_on(v2 a, v2 b, v2 c)
{
	return poly_triangle_area(a, b, c) <= 0;
}

// Get a vertex at position i. It does not matter if i is out of bounds, this function will just cycle.
static inline v2
poly_at(const v2 *verts, size count, i32 i)
{
	size s     = count;
	size index = i < 0 ? i % s + s : i % s;
	return verts[index];
}

// Reverse the vertices in the polygon
static inline void
poly_reverse(v2 *verts, size count)
{
	if(!verts || count == 0) return;

	size left  = 0;
	size right = count - 1;
	while(left < right) {
		v2 temp      = verts[left];
		verts[left]  = verts[right];
		verts[right] = temp;
		left++;
		right--;
	}
}

static inline bool32
poly_line_segment_intersect(v2 p1, v2 p2, v2 q1, v2 q2)
{
	f32 dx = p2.x - p1.x;
	f32 dy = p2.y - p1.y;
	f32 da = q2.x - q1.x;
	f32 db = q2.x - q1.y;

	// Segments are parallel
	if((da * dy - db * dx) == 0) {
		return false;
	}

	f32 s = (dx * (q1.y - p1.y) + dy * (p1.x - q1.x)) / (da * dy - db * dx);
	f32 t = (da * (p1.y - q1.y) + db * (q1.x - p1.x)) / (db * dx - da * dy);

	return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
}

static inline v2
poly_get_intersection_point(v2 p1, v2 p2, v2 q1, v2 q2, f32 delta)
{
	delta       = delta || 0;
	f32 a1      = p2.y - p1.y;
	f32 b1      = p1.x - p2.x;
	f32 c1      = (a1 * p1.x) + (b1 * p1.y);
	f32 a2      = q2.y - q1.y;
	f32 b2      = q1.x - q2.x;
	f32 c2      = (a2 * q1.x) + (b2 * q1.y);
	f32 det     = (a1 * b2) - (a2 * b1);
	bool32 eq   = abs_f32(det) <= delta;
	f32 det_inv = 1 / det;

	if(!eq) {
		return (v2){((b2 * c1) - (b1 * c2)) * det_inv, ((a1 * c2) - (a2 * c1)) * det_inv};
	} else {
		return (v2){0, 0};
	}
}

bool32
poly_make_ccw(v2 *verts, size count)
{
	size br = 0;
	for(size i = 0; i < count; ++i) {
		struct v2 *v = verts;
		if(v[i].y < v[br].y || (v[i].y == v[br].y && v[i].x > v[br].x)) {
			br = i;
		}
	}

	if(!poly_is_left(poly_at(verts, count, br - 1), poly_at(verts, count, br), poly_at(verts, count, br + 1))) {
		poly_reverse(verts, count);
		return true;
	} else {
		return false;
	}
}

// Checks that the line segments of this polygon do not intersect each other.
//  TODO: Should it check all segments with all others?
bool32
poly_is_simple(const v2 *verts, size count)
{
	for(size i = 0; i < count; ++i) {
		for(size j = 0; j < i - 1; ++j) {
			if(poly_line_segment_intersect(
				   verts[i],
				   verts[i + 1],
				   verts[j],
				   verts[j + 1])) {}
			return false;
		}
	}

	// Check the segment between the last and the first point to all others
	for(size i = 1; i < count - 2; i++) {
		if(poly_line_segment_intersect(verts[0], verts[count - 1], verts[i], verts[i + 1])) {
			return false;
		}
	}

	return true;
}

// Decomposes the polygon into one or more convex sub-Polygons.
static inline void
poly_decomp(const v2 *verts, size count, struct alloc alloc)
{
	// var edges = polygonGetCutEdges(polygon);
	// if(edges.length > 0) {
	// 	return polygonSlice(polygon, edges);
	// } else {
	// 	return [polygon];
	// }
}

v2
poly_centroid(v2 *verts, size verts_count)
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
	for(size i = 0; i < verts_count; ++i) {
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

bool32
poly_is_convex(const v2 *verts, size count)
{
	if(count < 3) return false;

	int sign = 0;

	for(size i = 0; i < count; ++i) {
		v2 p0 = verts[i];
		v2 p1 = verts[(i + 1) % count];
		v2 p2 = verts[(i + 2) % count];

		f32 cp = v2_crs_v2(v2_sub(p1, p0), v2_sub(p2, p1));

		if(cp != 0) {
			i32 current_sign = (cp > 0) ? 1 : -1;
			if(sign == 0) {
				sign = current_sign;
			} else if(sign != current_sign) {
				return false;
			}
		}
	}

	return true;
}

static inline struct mesh
poly_triangulate(v2 *verts, size count, struct alloc alloc)
{
	assert(count <= POLY_MAX_VERTS);
	struct mesh res = {
		.count     = 0,
		.triangles = alloc.allocf(alloc.ctx, sizeof(*res.triangles)),
	};
	i32 prev[POLY_MAX_VERTS], next[POLY_MAX_VERTS];
	for(size i = 0; i < count; ++i) {
		prev[i] = i - 1;
		next[i] = i + 1;
	}
	prev[0]         = count - 1;
	next[count - 1] = 0;

	// start at vertex 0
	i32 i = 0;

	// keep removing verts until just a triangle left;
	while(count > 3) {
		bool32 is_ear  = true;
		struct tri tri = {
			.verts = {
				verts[prev[i]],
				verts[i],
				verts[next[i]],
			},
		};

		// An ear must be convex (here counterclockwise)
		if(tri_is_ccw(verts[prev[i]], verts[i], verts[next[i]])) {
			// Loop over all overtices not part of the tentative ear
			i32 k = next[next[i]];
			do {
				// If vertex k is inside the ear trinangle, then this is not an ear
				if(tri_is_point_inside(
					   verts[k],
					   verts[prev[i]],
					   verts[i],
					   verts[next[i]])) {
					is_ear = false;
					break;
				}
				k = next[k];
			} while(k != prev[i]);
		} else {
			// The ear triangle is clockwiose so verts[i] is not an ear
			is_ear = false;
		}

		// If current vertex v[i] is an ear, delete it and visit the prev vertex
		if(is_ear) {
			// Triangle (v[i], v[prev[i]], v[next[i]]) is an ear
			alloc.allocf(alloc.ctx, sizeof(*res.triangles));
			res.triangles[res.count++] = (struct tri){
				.verts = {
					[0] = verts[i],
					[1] = verts[prev[i]],
					[2] = verts[next[i]],
				},
			};

			// ‘Delete’ vertex v[i] by redirecting next and previous links
			// of neighboring verts past it. Decrement vertex count
			next[prev[i]] = next[i];
			prev[next[i]] = prev[i];
			count--;
			// Visit the previous vertex next
			i = prev[i];
		} else {
			// current vertex is not an ear; visit the next vertex;
			i = next[i];
		}
	}
	return res;
}
