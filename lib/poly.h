#pragma once

#include "base/types.h"

#include "base/v2.h"
#include "base/arr.h"
#include "lib/tri.h"

#define POLY_MAX_VERTS 8

// https://github.com/schteppe/poly-decomp.js/blob/master/src/index.js

struct poly {
	size count;
	v2 verts[POLY_MAX_VERTS];
};

struct mesh {
	size count;
	struct poly *items;
};

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

// https://stackoverflow.com/questions/2792443/finding-the-centroid-of-a-polygon
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

b32
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

static inline b32
poly_make_ccw(v2 *verts, size count)
{
	size br = 0;

	// find bottom right point
	for(size i = 1; i < count; ++i) {
		if(verts[i].y < verts[br].y || (verts[i].y == verts[br].y && verts[i].x > verts[br].x)) {
			br = i;
		}
	}

	// reverse poly if clockwise
	if(!tri_is_ccw(
		   poly_at(verts, count, br - 1), // A
		   poly_at(verts, count, br),     // B
		   poly_at(verts, count, br + 1)  // C
		   )) {
		poly_reverse(verts, count);
		return true;
	} else {
		return false;
	}
}

// Check if three points are collinear
// Threshold angle to use when comparing the vectors. The function will return true if the angle between the resulting vectors is less than this value. Use zero for max precision.
static inline b32
poly_collinear(v2 a, v2 b, v2 c, f32 threshold_ang)
{
	if(!threshold_ang) {
		return tri_signed_2d_area(a, b, c) == 0;
	} else {
		v2 ab     = v2_sub(b, a);
		v2 bc     = v2_sub(c, b);
		f32 dot   = v2_dot(ab, bc);
		f32 mag_a = v2_len(ab);
		f32 mag_b = v2_len(bc);
		f32 angle = acos_f32(dot / (mag_a * mag_b));
		return angle < threshold_ang;
	}
}

static inline size
poly_remove_collinear_points(v2 *verts, size count, f32 precision)
{
	if(count <= 3) return count;

	size res = 0;
	for(size i = 0; i < count; ++i) {
		v2 prev = poly_at(verts, count, (i + count - 1));
		v2 curr = poly_at(verts, count, i);
		v2 next = poly_at(verts, count, (i + 1));

		if(!poly_collinear(prev, curr, next, precision)) {
			verts[res++] = curr;
		}
	}

	// Ensure we still have a valid polygon
	if(res < 3) return count;

	return res;
}

static inline struct mesh
poly_triangulate(v2 *verts, size count, struct alloc alloc, struct alloc scratch)
{
	poly_make_ccw(verts, count);
	// Linked list
	i32 *prev       = arr_new(prev, count, scratch);
	i32 *next       = arr_new(next, count, scratch);
	struct mesh res = {
		.count = 0,
		.items = alloc.allocf(alloc.ctx, sizeof(*res.items)),
	};
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
		b32 is_ear = true;

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
			alloc.allocf(alloc.ctx, sizeof(*res.items));
			res.items[res.count++] = (struct poly){
				.count = 3,
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

	alloc.allocf(alloc.ctx, sizeof(*res.items));
	res.items[res.count++] = (struct poly){
		.count = 3,
		.verts = {
			[0] = verts[i],
			[1] = verts[prev[i]],
			[2] = verts[next[i]],
		},
	};

	return res;
}

// Hertel Mehlhorn
// TODO: currently we allocate more polygons than what we need
// We could maybe use the system of the poly_triangulate double link list thing to remove polys easier
static inline struct mesh
poly_decomp_hm(struct mesh mesh, struct alloc alloc, struct alloc scratch)
{
	b32 *merged = scratch.allocf(scratch.ctx, sizeof(*merged) * mesh.count);
	mclr(merged, sizeof(*merged) * mesh.count);

	struct mesh res = {
		.items = alloc.allocf(alloc.ctx, sizeof(*res.items) * mesh.count),
	};

	for(size i = 0; i < mesh.count; i++) {
		if(merged[i]) { continue; }

		struct poly *a = mesh.items + i;
		bool did_merge = false;

		for(size j = i + 1; j < mesh.count; ++j) {
			if(merged[j]) { continue; }

			struct poly *b = mesh.items + j;

			// Try to merge a and b if they share and edge
			// Find the shared edge (reverse order in both)
			for(size ai = 0; ai < a->count; ++ai) {
				v2 a0 = a->verts[ai];
				v2 a1 = a->verts[(ai + 1) % a->count];

				for(size bi = 0; bi < b->count; ++bi) {
					v2 b1 = b->verts[bi];
					v2 b0 = b->verts[(bi + 1) % b->count];

					// They share an edge
					if((a0.x == b0.x && a0.y == b0.y) && (a1.x == b1.x && a1.y == b1.y)) {
						// create new merged polygon
						struct poly merged_poly = {0};

						// Add a verts up to a0
						{
							size idx = (ai + 1) % a->count;
							while(!v2_eq(a->verts[idx], a0)) {
								merged_poly.verts[merged_poly.count++] = a->verts[idx];
								idx                                    = (idx + 1) % a->count;
							}
						}

						// Add verts from b (excluding shared edge b1→b0)
						{
							size idx = (bi + 1) % b->count;
							while(!v2_eq(b->verts[idx], b1)) {
								merged_poly.verts[merged_poly.count++] = b->verts[idx];
								idx                                    = (idx + 1) % b->count;
							}
						}

						dbg_assert(merged_poly.count > 2);

						if(poly_is_convex(merged_poly.verts, merged_poly.count)) {
							res.items[res.count++] = merged_poly;
							merged[i] = merged[j] = true;
							did_merge             = true;
							break;
						}
					}
				}
				if(did_merge) { break; }
			}
		}

		if(!did_merge) {
			// keep triangle as is
			res.items[res.count++] = *a;
		}
	}

	return res;
}

// Decomposes the polygon into one or more convex sub-Polygons.
static inline struct mesh
poly_decomp(v2 *verts, size count, struct alloc alloc, struct alloc scratch)
{
	struct mesh mesh = poly_triangulate(verts, count, scratch, scratch);
	struct mesh res  = poly_decomp_hm(mesh, alloc, scratch);
	return res;
}
