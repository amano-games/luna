#pragma once

#include "arr.h"
#include "handles.h"
#include "sys-utils.h"
#include "sys-assert.h"

#define CUTE_C2_IMPLEMENTATION
#include "cute_c2.h"

#include "sys-types.h"

#define COL_MAX_POLYGON_VERTS 8
#define COL_MAX_SUB_POLY      10

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

struct narrow_collision {
	struct col_manifold m;
	struct entity_handle handles[2];
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

struct col_capsule {
	v2 a;
	v2 b;
	f32 ra;
	f32 rb;
	// Distance between ends
	f32 d;
	struct col_tangents tangents;
};

struct sub_poly {
	int count;
	v2 verts[COL_MAX_POLYGON_VERTS];
	v2 norms[COL_MAX_POLYGON_VERTS];
};

struct col_poly {
	usize count;
	struct sub_poly sub_polys[COL_MAX_SUB_POLY];
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

static inline c2v
v2_to_c2v(v2 v)
{
	c2v r = (c2v){v.x, v.y};
	return r;
}

static inline v2
c2v_to_v2(c2v v)
{
	v2 r = (v2){v.x, v.y};
	return r;
}

static inline c2Circle
cir_to_c2cir(struct col_cir v)
{
	return (c2Circle){
		.r = v.r,
		.p = v2_to_c2v(v.p)};
}

static inline c2AABB
aabb_to_c2aabb(struct col_aabb v)
{
	return (c2AABB){
		.min = v2_to_c2v(v.min),
		.max = v2_to_c2v(v.max)};
}

static inline c2Poly
poly_to_c2poly(struct sub_poly v)
{
	c2Poly r = {
		.count = v.count,
	};

	for(int i = 0; i < v.count; ++i) {
		r.verts[i] = v2_to_c2v(v.verts[i]);
		r.norms[i] = v2_to_c2v(v.norms[i]);
	}
	return r;
}

static inline void
c2toi_to_toi(c2TOIResult *c2toi, struct col_toi *toi)
{
	toi->hit        = c2toi->hit;
	toi->toi        = c2toi->toi;
	toi->n          = c2v_to_v2(c2toi->n);
	toi->p          = c2v_to_v2(c2toi->p);
	toi->iterations = c2toi->iterations;
}

static inline void
c2manifold_to_manifold(c2Manifold *c2m, struct col_manifold *m)
{
	m->count             = c2m->count;
	m->depth             = c2m->depths[0];
	m->normal            = c2v_to_v2(c2m->n);
	m->contact_points[0] = c2v_to_v2(c2m->contact_points[0]);
	m->contact_points[1] = c2v_to_v2(c2m->contact_points[1]);
}

struct col_cir col_merge_circles(struct col_cir a, struct col_cir b);
void col_poly_init(struct col_poly *p);
v2 col_poly_centroid(struct col_poly *p);
struct col_cir col_capsule_get_circle_col(struct col_capsule capsule, v2 p, f32 *t, v2 *closest);

void col_point_to_line(v2 c, v2 a, v2 b, f32 *t, v2 *d);
// TODO: Move from col shape to destructured params
int col_circle_to_circle(struct col_cir a, struct col_cir b);
int col_circle_to_aabb(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2);
int col_circle_to_capsule(struct col_cir a, struct col_capsule b);
int col_circle_to_poly(struct col_cir a, struct col_poly b);

int col_aabb_to_aabb(f32 x1a, f32 y1a, f32 x2a, f32 y2a, f32 x1b, f32 y1b, f32 x2b, f32 y2b);
struct col_toi col_circle_toi(struct col_cir a, v2 va, struct col_shape b, v2 vb);

void col_circle_to_circle_manifold(struct col_cir a, struct col_cir b, struct col_manifold *m);
void col_circle_to_aabb_manifold(struct col_cir a, struct col_aabb b, struct col_manifold *m);
void col_circle_to_capsule_manifold(struct col_cir a, struct col_capsule b, struct col_manifold *m, f32 *t, v2 *closest);
void col_circle_to_poly_manifold(struct col_cir a, struct col_poly b, struct col_manifold *m);
