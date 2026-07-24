#define CUTE_C2_IMPLEMENTATION
#include "cute_c2.h"

#include "collisions.h"

#include "base/dbg.h"
#include "base/types.h"

#include "base/mathfunc.h"
#include "lib/tri.h"
#include "base/v2.h"

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
poly_to_c2poly(struct col_poly v)
{
	c2Poly r = {
		.count = v.count,
	};

	for(ssize i = 0; i < v.count; ++i) {
		r.verts[i] = v2_to_c2v(v.verts[i]);
		r.norms[i] = v2_to_c2v(v.norms[i]);
	}
	return r;
}

static inline struct col_poly
c2poly_to_poly(struct c2Poly v)
{
	struct col_poly r = {
		.count = v.count,
	};

	for(int i = 0; i < v.count; ++i) {
		r.verts[i] = c2v_to_v2(v.verts[i]);
		r.norms[i] = c2v_to_v2(v.norms[i]);
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

static inline c2x
col_transform_to_c2x(const struct col_transform *bx)
{
	struct col_transform x = bx ? *bx : col_transform_identity();
	return (c2x){
		.p = {x.p.x, x.p.y},
		.r = {x.r.c, x.r.s},
	};
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

void
col_poly_init(struct col_poly *p)
{
	c2Poly c2p = poly_to_c2poly(*p);
	c2MakePoly(&c2p);
	*p = c2poly_to_poly(c2p);
}

struct col_cir
col_merge_circles(struct col_cir a, struct col_cir b)
{
	struct col_cir res = {0};
	v2 d               = v2_sub(b.p, a.p);
	f32 dist2          = v2_dot(d, d);
	f32 r_diff         = b.r - a.r;

	if((r_diff * r_diff) >= dist2) {
		// The shere with the larger radius encloses the other;
		// just set s to be the larget of the two spheres
		res = (b.r >= a.r) ? b : a;
	} else {
		// Spheres partially overlapping or disjointed
		f32 dist = sqrt_f32(dist2);
		res.r    = (dist + a.r + b.r) * 0.5f;
		res.p    = a.p;
		if(dist > EPSILON) {
			f32 r_a = (res.r - a.r) / dist;
			res.p   = v2_add(res.p, v2_mul(d, r_a));
		}
	}
	return res;
}

int
col_point_to_aabb(f32 xa, f32 ya, f32 x1b, f32 y1b, f32 x2b, f32 y2b)
{
	return xa > x1b && xa < x2b && ya > y1b && ya < y2b;
}

int
col_circle_to_circle(f32 ax, f32 ay, f32 ar, f32 bx, f32 by, f32 br)
{
	c2Circle c2a = {.p = {ax, ay}, .r = ar};
	c2Circle c2b = {.p = {bx, by}, .r = br};
	int r        = c2CircletoCircle(c2a, c2b);
	return r;
}

int
col_circle_to_aabb(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2)
{
	c2Circle c2a =
		{
			.p = (c2v){x, y},
			.r = r,
		};
	c2AABB c2b = {.min = (c2v){x1, y1}, .max = (c2v){x2, y2}};
	int res    = c2CircletoAABB(c2a, c2b);
	return res;
}

struct col_cir
col_capsule_get_circle_col(struct col_capsule capsule, f32 x, f32 y)
{
	v2 closest           = {0};
	v2 closest_tangent_p = {0};
	f32 t                = 0.0;
	v2 p                 = {x, y};
	{
		// First we get the closest point from the ball to the tangent A
		v2 ta = {0};
		col_point_to_line(p, capsule.tangents.a.a, capsule.tangents.a.b, NULL, &ta);

		// Closes point from the ball to tangent B
		v2 tb = {0};
		col_point_to_line(p, capsule.tangents.b.a, capsule.tangents.b.b, NULL, &tb);

		// Get the vector from the ball to the tangents
		v2 tap = v2_sub(p, ta);
		v2 tbp = v2_sub(p, tb);

		f32 tal = v2_len_sq(tap);
		f32 tbl = v2_len_sq(tbp);

		// The closes point from a tangent to the ball
		closest_tangent_p = tal < tbl ? ta : tb;
	}

	col_point_to_line(closest_tangent_p, capsule.a.p, capsule.b.p, &t, &closest);
	struct col_cir res = {
		.p = {closest.x, closest.y},
		.r = lerp(capsule.a.r, capsule.b.r, t),
	};

	return res;
}

i32
col_circle_to_capsule(f32 x, f32 y, f32 r, struct col_capsule b)
{
	struct col_cir circle_b = col_capsule_get_circle_col(b, x, y);
	i32 res                 = col_circle_to_circle(
		x,
		y,
		r,
		circle_b.p.x,
		circle_b.p.y,
		circle_b.r);

	return res;
}

int
col_circle_to_poly(struct col_cir a, struct col_poly b, struct col_transform *bx)
{
	c2Circle c2a = cir_to_c2cir(a);
	int res      = 0;
	c2Poly c2b   = poly_to_c2poly(b);
	c2x cbx      = col_transform_to_c2x(bx);
	res          = c2CircletoPoly(c2a, &c2b, &cbx);
	return res;
}

int
col_aabb_to_aabb(
	f32 x1a,
	f32 y1a,
	f32 x2a,
	f32 y2a,
	f32 x1b,
	f32 y1b,
	f32 x2b,
	f32 y2b)
{
	c2AABB c2a = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2AABB c2b = {.min = {x1b, y1b}, .max = {x2b, y2b}};

	int r = c2AABBtoAABB(c2a, c2b);
	return r;
}

int
col_aabb_to_poly(f32 x1a, f32 y1a, f32 x2a, f32 y2a, struct col_poly b)
{
	// WARN: slow because we do the manifold
	// https://github.com/RandyGaul/cute_headers/issues/404
	c2AABB c2a   = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	int res      = 0;
	c2Manifold m = {0};
	c2Poly c2b   = poly_to_c2poly(b);
	c2AABBtoPolyManifold(c2a, &c2b, NULL, &m);
	res = m.count > 0;
	return res;
}

struct col_toi
col_circle_toi(struct col_cir a, v2 va, struct col_shape b, v2 vb)
{
	dbg_sentinel("col");
	c2Circle c2a   = cir_to_c2cir(a);
	c2v c2va       = v2_to_c2v(va);
	C2_TYPE type_b = C2_TYPE_POLY;
	c2v c2vb       = v2_to_c2v(vb);
	void *c2b      = NULL;
	c2Poly c2_poly;
	c2Circle c2_cir;

	switch(b.type) {
	case COL_TYPE_AABB: {
		type_b = C2_TYPE_AABB;
		dbg_assert(false); // not implemented
	} break;
	case COL_TYPE_CIR: {
		type_b = C2_TYPE_CIRCLE;
		c2_cir = cir_to_c2cir(b.cir);
		c2b    = (void *)&c2_cir;
	} break;
	case COL_TYPE_POLY: {
		dbg_not_implemeneted("col");
	} break;
	default: {
		dbg_assert(false);
	};
	}

	c2TOIResult res = c2TOI(
		(void *)&c2a,
		C2_TYPE_CIRCLE,
		NULL,
		c2va,
		c2b,
		type_b,
		NULL,
		c2vb,
		true);
	struct col_toi toi = {0};
	c2toi_to_toi(&res, &toi);

	return toi;

error:
	return (struct col_toi){0};
}

void
col_circle_to_circle_manifold(
	f32 ax, f32 ay, f32 ar, f32 bx, f32 by, f32 br, struct col_manifold *m)
{
	c2Circle c2a   = {.p = {ax, ay}, .r = ar};
	c2Circle c2b   = {.p = {bx, by}, .r = br};
	c2Manifold res = {0};
	c2CircletoCircleManifold(c2a, c2b, &res);
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_aabb_manifold(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2, struct col_manifold *m)
{
	c2Circle c2a   = {.p = {x, y}, .r = r};
	c2AABB c2b     = {.min = {x1, y1}, .max = {x2, y2}};
	c2Manifold res = {0};
	c2CircletoAABBManifold(c2a, c2b, &res);
	c2manifold_to_manifold(&res, m);
}

void
col_aabb_to_circle_manifold(f32 x1a, f32 y1a, f32 x2a, f32 y2a, f32 bx, f32 by, f32 br, struct col_manifold *m)
{
	c2AABB c2a     = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2Circle c2b   = {.p = {bx, by}, .r = br};
	c2Manifold res = {0};
	c2CircletoAABBManifold(c2b, c2a, &res);
	res.n.x = -res.n.x;
	res.n.y = -res.n.y;
	c2manifold_to_manifold(&res, m);
}

void
col_aabb_to_aabb_manifold(f32 x1a, f32 y1a, f32 x2a, f32 y2a, f32 x1b, f32 y1b, f32 x2b, f32 y2b, struct col_manifold *m)
{
	c2AABB c2a     = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2AABB c2b     = {.min = {x1b, y1b}, .max = {x2b, y2b}};
	c2Manifold res = {0};

	c2AABBtoAABBManifold(c2a, c2b, &res);
	c2manifold_to_manifold(&res, m);
}

void
col_aabb_to_poly_manifold(
	f32 x1a,
	f32 y1a,
	f32 x2a,
	f32 y2a,
	struct col_poly b,
	struct col_transform *bx,
	struct col_manifold *m)
{
	c2AABB c2a     = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2Manifold res = {0};
	c2Poly c2b     = poly_to_c2poly(b);
	c2x cbx        = col_transform_to_c2x(bx);
	c2AABBtoPolyManifold(c2a, &c2b, &cbx, &res);
	c2manifold_to_manifold(&res, m);
}

void
col_poly_to_poly_manifold(
	struct col_poly a,
	struct col_transform *ax,
	struct col_poly b,
	struct col_transform *bx,
	struct col_manifold *m)
{
	c2Manifold res = {0};
	c2Poly c2a     = poly_to_c2poly(a);
	c2Poly c2b     = poly_to_c2poly(b);
	c2x cax        = col_transform_to_c2x(ax);
	c2x cbx        = col_transform_to_c2x(bx);
	c2PolytoPolyManifold(&c2a, &cax, &c2b, &cbx, &res);
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_capsule_manifold(
	f32 x,
	f32 y,
	f32 r,

	f32 x1b,
	f32 y1b,
	f32 r1b,

	f32 x2b,
	f32 y2b,
	f32 r2b,

	f32 t1ax,
	f32 t1ay,
	f32 t1bx,
	f32 t1by,

	f32 t2ax,
	f32 t2ay,
	f32 t2bx,
	f32 t2by,

	struct col_manifold *m)
{
	struct col_capsule b = {
		.a = {
			.p.x = x1b,
			.p.y = y1b,
			.r   = r1b,
		},
		.b = {
			.p.x = x2b,
			.p.y = y2b,
			.r   = r2b,
		},
		.tangents = {
			.a.a.x = t1ax,
			.a.a.y = t1ay,
			.a.b.x = t1bx,
			.a.b.y = t1by,

			.b.a.x = t2ax,
			.b.a.y = t2ay,
			.b.b.x = t2bx,
			.b.b.y = t2by,
		}};
	struct col_cir b_cir = col_capsule_get_circle_col(b, x, y);
	struct c2Circle c2b  = cir_to_c2cir(b_cir);
	c2Circle c2a         = {.p.x = x, .p.y = y, .r = r};
	c2Manifold res       = {0};
	c2CircletoCircleManifold(c2a, c2b, &res);
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_poly_manifold(f32 x, f32 y, f32 r, struct col_poly b, struct col_transform *bx, struct col_manifold *m)
{
	c2Circle c2a   = {.p.x = x, .p.y = y, .r = r};
	c2Manifold c2m = {0};
	c2Poly c2b     = poly_to_c2poly(b);
	c2x cbx        = col_transform_to_c2x(bx);
	c2CircletoPolyManifold(c2a, &c2b, &cbx, &c2m);
	c2manifold_to_manifold(&c2m, m);
}

// TODO: optimize division
// If divisions are expensive, the division operation can be postponed by multiply-
// ing both sides of the comparisons by the denominator, which as a square term is
// guaranteed to be nonnegative.
f32
col_point_to_line_t(v2 c, v2 a, v2 b)
{
	f32 res = 0.0f;
	v2 ab   = v2_sub(b, a);
	v2 ac   = v2_sub(c, a);
	f32 t   = v2_dot(ac, ab) / v2_dot(ab, ab); // project c onto ab, computing parametrized position
	res     = clamp_f32(t, 0.0f, 1.0f);        // clamp t if outside of segment
	return res;
}

void
col_point_to_line(v2 c, v2 a, v2 b, f32 *const t_out, v2 *const d_out)
{
	v2 ab = v2_sub(b, a);
	v2 ac = v2_sub(c, a);
	v2 d  = {0};
	f32 t = col_point_to_line_t(c, a, b);

	// Compute projected position from the clamped t
	d = v2_add(a, v2_mul(ab, t));
	if(t_out) { *t_out = t; }
	if(d_out) { *d_out = d; }
}

int
col_point_to_tri(f32 x, f32 y, f32 xa, f32 ya, f32 xb, f32 yb, f32 xc, f32 yc)
{
	v2 p  = {x, y};
	v2 a  = {xa, ya};
	v2 b  = {xb, yb};
	v2 c  = {xc, yc};
	f32 u = 0;
	f32 v = 0;
	f32 w = 0;
	tri_barycentric(p, a, b, c, &u, &v, &w);

	return v >= 0.0f && w >= 0.0f && (v + w) <= 1.0f;
}

struct col_aabb
col_shapes_get_bounding_box(struct col_shapes shapes)
{
	struct col_aabb res = {
		.min.x = F32_MAX,
		.min.y = F32_MAX,
		.max.x = F32_MIN,
		.max.y = F32_MIN,
	};

	for(ssize i = 0; i < shapes.count; ++i) {
		struct col_aabb aabb = col_shape_get_bounding_box(shapes.items[i]);
		res.min.x            = min_f32(res.min.x, aabb.min.x);
		res.min.y            = min_f32(res.min.y, aabb.min.y);
		res.max.x            = max_f32(res.max.x, aabb.max.x);
		res.max.y            = max_f32(res.max.y, aabb.max.y);
	}

	return res;
}

static inline struct col_aabb
col_cir_get_bounding_box(struct col_cir col)
{
	struct col_aabb res = {
		.min = {col.p.x - col.r, col.p.y - col.r},
		.max = {col.p.x + col.r, col.p.y + col.r},
	};
	return res;
}

static inline struct col_aabb
col_poly_get_bounding_box(struct col_poly poly)
{
	struct col_aabb res = {
		.min.x = F32_MAX,
		.min.y = F32_MAX,
		.max.x = F32_MIN,
		.max.y = F32_MIN,
	};

	for(ssize j = 0; j < poly.count; ++j) {
		v2 p      = poly.verts[j];
		res.min.x = min_f32(res.min.x, p.x);
		res.min.y = min_f32(res.min.y, p.y);
		res.max.x = max_f32(res.max.x, p.x);
		res.max.y = max_f32(res.max.y, p.y);
	}

	return res;
}

// TODO: accept a min/max angle of rotation so the boundig box is smaller
static inline struct col_aabb
col_capsule_get_bounding_box(struct col_capsule col)
{
	struct col_aabb res = {
		.min.x = F32_MAX,
		.min.y = F32_MAX,
		.max.x = F32_MIN,
		.max.y = F32_MIN,
	};

	struct col_cir cir = col_merge_circles(col.a, col.b);
	res                = col_cir_get_bounding_box(cir);

	return res;
}

struct col_aabb
col_shape_get_bounding_box(struct col_shape shape)
{
	struct col_aabb res = {0};
	dbg_assert(shape.type != COL_TYPE_NONE);
	switch(shape.type) {
	case COL_TYPE_CIR: {
		res = (struct col_aabb){
			.min = {shape.cir.p.x - shape.cir.r, shape.cir.p.y - shape.cir.r},
			.max = {shape.cir.p.x + shape.cir.r, shape.cir.p.y + shape.cir.r},
		};
	} break;
	case COL_TYPE_AABB: {
		res = shape.aabb;
	} break;
	case COL_TYPE_POLY: {
		res = col_poly_get_bounding_box(shape.poly);
	} break;
	case COL_TYPE_CAPSULE: {
		res = col_capsule_get_bounding_box(shape.capsule);
	} break;
	default: {
		dbg_sentinel("col");
	}
	}
	return res;

error:
	return (struct col_aabb){0};
}

void
col_to_col_manifold(
	struct col_shape *a,
	struct col_transform a_transform,
	struct col_shape *b,
	struct col_transform b_transform,
	struct col_manifold *m)
{
	switch(a->type) {
	case COL_TYPE_CIR: {
		switch(b->type) {
		case COL_TYPE_CIR: {
			col_circle_to_circle_manifold(
				a->cir.p.x + a_transform.p.x,
				a->cir.p.y + a_transform.p.y,
				a->cir.r,
				b->cir.p.x + b_transform.p.x,
				b->cir.p.y + b_transform.p.y,
				b->cir.r,
				m);
		} break;
		case COL_TYPE_AABB: {
			col_circle_to_aabb_manifold(
				a->cir.p.x + a_transform.p.x,
				a->cir.p.y + a_transform.p.y,
				a->cir.r,
				b->aabb.min.x + b_transform.p.x,
				b->aabb.min.y + b_transform.p.y,
				b->aabb.max.x + b_transform.p.x,
				b->aabb.max.y + b_transform.p.y,
				m);
		} break;
		case COL_TYPE_POLY: {
			col_circle_to_poly_manifold(
				a->cir.p.x + a_transform.p.x,
				a->cir.p.y + a_transform.p.y,
				a->cir.r,
				b->poly,
				&b_transform,
				m);
		} break;
		case COL_TYPE_CAPSULE: {
			col_circle_to_capsule_manifold(
				a->cir.p.x + a_transform.p.x,
				a->cir.p.y + a_transform.p.y,
				a->cir.r,

				b->capsule.a.p.x + b_transform.p.x,
				b->capsule.a.p.y + b_transform.p.y,
				b->capsule.a.r,

				b->capsule.b.p.x + b_transform.p.x,
				b->capsule.b.p.y + b_transform.p.y,
				b->capsule.b.r,

				b->capsule.tangents.a.a.x + b_transform.p.x,
				b->capsule.tangents.a.a.y + b_transform.p.y,
				b->capsule.tangents.a.b.x + b_transform.p.x,
				b->capsule.tangents.a.b.y + b_transform.p.y,

				b->capsule.tangents.b.a.x + b_transform.p.x,
				b->capsule.tangents.b.a.y + b_transform.p.y,
				b->capsule.tangents.b.b.x + b_transform.p.x,
				b->capsule.tangents.b.b.y + b_transform.p.y,

				m);
		} break;
		default: {
			dbg_sentinel("dbg_shape");
		} break;
		}
	} break;
	case COL_TYPE_AABB: {
		switch(b->type) {
		case COL_TYPE_CIR: {
			col_aabb_to_circle_manifold(
				a->aabb.min.x + a_transform.p.x,
				a->aabb.min.y + a_transform.p.y,
				a->aabb.max.x + a_transform.p.x,
				a->aabb.max.y + a_transform.p.y,
				b->cir.p.x + b_transform.p.x,
				b->cir.p.y + b_transform.p.y,
				b->cir.r,
				m);
		} break;
		case COL_TYPE_AABB: {
			col_aabb_to_aabb_manifold(
				a->aabb.min.x + a_transform.p.x,
				a->aabb.min.y + a_transform.p.y,
				a->aabb.max.x + a_transform.p.x,
				a->aabb.max.y + a_transform.p.y,
				b->aabb.min.x + b_transform.p.x,
				b->aabb.min.y + b_transform.p.y,
				b->aabb.max.x + b_transform.p.x,
				b->aabb.max.y + b_transform.p.y,
				m);
		} break;
		case COL_TYPE_POLY: {
			col_aabb_to_poly_manifold(
				a->aabb.min.x + a_transform.p.x,
				a->aabb.min.y + a_transform.p.y,
				a->aabb.max.x + a_transform.p.x,
				a->aabb.max.y + a_transform.p.y,
				b->poly,
				&b_transform,
				m);
		} break;
		case COL_TYPE_CAPSULE: {
		} break;
		default: {
			dbg_sentinel("dbg_shape");
		} break;
		}
	} break;
	case COL_TYPE_POLY: {
		switch(b->type) {
		case COL_TYPE_CIR: {
			col_circle_to_poly_manifold(
				b->cir.p.x + b_transform.p.x,
				b->cir.p.y + b_transform.p.y,
				b->cir.r,
				a->poly,
				&a_transform,
				m);
			m->normal.x = -m->normal.x;
			m->normal.y = -m->normal.y;
		} break;
		case COL_TYPE_AABB: {
			col_aabb_to_poly_manifold(
				b->aabb.min.x + b_transform.p.x,
				b->aabb.min.y + b_transform.p.y,
				b->aabb.max.x + b_transform.p.x,
				b->aabb.max.y + b_transform.p.y,
				a->poly,
				&a_transform,
				m);
			m->normal.x = -m->normal.x;
			m->normal.y = -m->normal.y;
		} break;
		case COL_TYPE_POLY: {
			col_poly_to_poly_manifold(
				a->poly,
				&a_transform,
				b->poly,
				&b_transform,
				m);
		} break;
		default: {
			dbg_sentinel("invalid collision shape");
		} break;
		}
	} break;
	case COL_TYPE_CAPSULE: {
		switch(b->type) {
		case COL_TYPE_CIR: {
			col_circle_to_capsule_manifold(
				b->cir.p.x + b_transform.p.x,
				b->cir.p.y + b_transform.p.y,
				b->cir.r,

				a->capsule.a.p.x + a_transform.p.x,
				a->capsule.a.p.y + a_transform.p.y,
				a->capsule.a.r,

				a->capsule.b.p.x + a_transform.p.x,
				a->capsule.b.p.y + a_transform.p.y,
				a->capsule.b.r,

				a->capsule.tangents.a.a.x + a_transform.p.x,
				a->capsule.tangents.a.a.y + a_transform.p.y,
				a->capsule.tangents.a.b.x + a_transform.p.x,
				a->capsule.tangents.a.b.y + a_transform.p.y,

				a->capsule.tangents.b.a.x + a_transform.p.x,
				a->capsule.tangents.a.a.y + a_transform.p.y,
				a->capsule.tangents.b.b.x + a_transform.p.x,
				a->capsule.tangents.b.b.y + a_transform.p.y,
				m);
			m->normal.x = -m->normal.x;
			m->normal.y = -m->normal.y;
		} break;
		case COL_TYPE_AABB: {
		} break;
		case COL_TYPE_POLY: {
		} break;
		case COL_TYPE_CAPSULE: {
		} break;
		default: {
			dbg_sentinel("invalid collision shape");
		} break;
		}
	} break;
	default: {
		dbg_sentinel("invalid collision shape");
	} break;
	}

error:;
}
