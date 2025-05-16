#pragma once

#define CUTE_C2_IMPLEMENTATION
#include "cute_c2.h"

#include "sys-types.h"

#define COL_MAX_POLYGON_VERTS 8
#define COL_SHAPES_MAX        10

enum col_type {
	COL_TYPE_NONE,
	COL_TYPE_CIR,
	COL_TYPE_AABB,
	COL_TYPE_POLY,
	COL_TYPE_CAPSULE,
};

struct col_manifold {
	int count;
	f32 depth;
	v2 normal;
	v2 contact_points[2];
};

struct col_toi {
	int hit;        // 1 if shapes were touching at the TOI, 0 if they never hit.
	f32 toi;        // The time of impact between two shapes.
	v2 n;           // Surface normal from shape A to B at the time of impact.
	v2 p;           // Point of contact between shapes A and B at time of impact.
	int iterations; // Number of iterations the solver underwent.
};

struct col_cir {
	f32 r;
	v2 p;
};

struct col_aabb {
	v2 min;
	v2 max;
};

struct col_line {
	v2 a;
	v2 b;
};

struct col_tangents {
	struct col_line a;
	struct col_line b;
};

struct col_poly {
	size count;
	v2 verts[COL_MAX_POLYGON_VERTS];
	v2 norms[COL_MAX_POLYGON_VERTS];
};

struct col_capsule {
	union {
		struct col_cir cirs[2];
		struct {
			struct col_cir a, b;
		};
	};
	f32 d;
	struct col_tangents tangents;
};

struct col_shape {
	enum col_type type;
	union {
		struct col_cir cir;
		struct col_aabb aabb;
		struct col_capsule capsule;
		struct col_poly poly;
	};
};

struct col_shapes {
	size count;
	struct col_shape items[COL_SHAPES_MAX];
};

static inline f32
col_aabb_w(struct col_aabb aabb)
{
	return aabb.max.x - aabb.min.x;
}

static inline f32
col_aabb_h(struct col_aabb aabb)
{
	return aabb.max.y - aabb.min.y;
}

struct col_cir col_merge_circles(struct col_cir a, struct col_cir b);
void col_poly_init(struct col_poly *p);
struct col_cir col_capsule_get_circle_col(struct col_capsule capsule, v2 p, f32 *t, v2 *closest);
struct col_aabb col_shape_get_bounding_box(struct col_shape shape);

void col_point_to_line(v2 c, v2 a, v2 b, f32 *t, v2 *d);
int col_point_to_tri(f32 x, f32 y, f32 xa, f32 ya, f32 xb, f32 yb, f32 xc, f32 yc);
int col_point_to_aabb(f32 xa, f32 ya, f32 x1b, f32 y1b, f32 x2b, f32 y2b);
int col_circle_to_circle(f32 ax, f32 ay, f32 ar, f32 bx, f32 by, f32 br);
int col_circle_to_aabb(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2);
int col_circle_to_capsule(struct col_cir a, struct col_capsule b);
int col_circle_to_poly(struct col_cir a, struct col_poly b);

int col_aabb_to_aabb(f32 x1a, f32 y1a, f32 x2a, f32 y2a, f32 x1b, f32 y1b, f32 x2b, f32 y2b);
int col_aabb_to_poly(f32 x1a, f32 y1a, f32 x2a, f32 y2a, struct col_poly b);
struct col_toi col_circle_toi(struct col_cir a, v2 va, struct col_shape b, v2 vb);

void col_circle_to_circle_manifold(f32 ax, f32 ay, f32 ar, f32 bx, f32 by, f32 br, struct col_manifold *m);
void col_circle_to_aabb_manifold(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2, struct col_manifold *m);
void col_circle_to_capsule_manifold(struct col_cir a, struct col_capsule b, struct col_manifold *m, f32 *t, v2 *closest);
void col_circle_to_poly_manifold(struct col_cir a, struct col_poly b, struct col_manifold *m);
void col_aabb_to_aabb_manifold(f32 x1a, f32 y1a, f32 x2a, f32 y2a, f32 x1b, f32 y1b, f32 x2b, f32 y2b, struct col_manifold *m);
void col_aabb_to_poly_manifold(f32 x1a, f32 y1a, f32 x2a, f32 y2a, struct col_poly p, struct col_manifold *m);
