#include "collisions.h"

#include "sys-assert.h"
#include "sys-types.h"

#include "trace.h"
#include "mathfunc.h"
#include "tri.h"
#include "v2.h"

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

	for(size i = 0; i < v.count; ++i) {
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

static inline void
c2manifold_to_manifold(c2Manifold *c2m, struct col_manifold *m)
{
	m->count             = c2m->count;
	m->depth             = c2m->depths[0];
	m->normal            = c2v_to_v2(c2m->n);
	m->contact_points[0] = c2v_to_v2(c2m->contact_points[0]);
	m->contact_points[1] = c2v_to_v2(c2m->contact_points[1]);
}

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

static inline v2
col_aabb_cntr(struct col_aabb aabb)
{
	f32 w = col_aabb_w(aabb);
	f32 h = col_aabb_h(aabb);

	v2 res = {
		.x = aabb.min.x + (w * 0.5f),
		.y = aabb.min.y + (h * 0.5f),
	};
	return res;
}

static inline rec_i32
col_aabb_to_rec_i32(struct col_aabb aabb)
{
	f32 w = col_aabb_w(aabb);
	f32 h = col_aabb_h(aabb);

	rec_i32 res = {
		.x = aabb.min.x,
		.y = aabb.min.y,
		.w = w,
		.h = h,
	};
	return res;
}

void
col_poly_init(struct col_poly *p)
{
	TRACE_START(__func__);

	c2Poly c2p = poly_to_c2poly(*p);
	c2MakePoly(&c2p);
	*p = c2poly_to_poly(c2p);

	TRACE_END();
}

struct col_cir
col_merge_circles(struct col_cir a, struct col_cir b)
{
	TRACE_START(__func__);
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
	TRACE_END();
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
	TRACE_START(__func__);
	c2Circle c2a = {.p = {ax, ay}, .r = ar};
	c2Circle c2b = {.p = {bx, by}, .r = br};
	int r        = c2CircletoCircle(c2a, c2b);
	TRACE_END();
	return r;
}

int
col_circle_to_aabb(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2)
{
	TRACE_START(__func__);
	c2Circle c2a =
		{
			.p = (c2v){x, y},
			.r = r,
		};
	c2AABB c2b = {.min = (c2v){x1, y1}, .max = (c2v){x2, y2}};
	int res    = c2CircletoAABB(c2a, c2b);
	TRACE_END();
	return res;
}

struct col_cir
col_capsule_get_circle_col(struct col_capsule capsule, v2 p, f32 *t, v2 *closest)
{
	TRACE_START(__func__);
	closest->x = 0;
	closest->y = 0;
	*t         = 0.0f;

	v2 closest_p = {0};
	{
		// We don't need t so we just use a temp variable
		f32 t_temp = 0;

		// First we get the closest point from the ball to the tangent A
		v2 ta = {0};
		col_point_to_line(p, capsule.tangents.a.a, capsule.tangents.a.b, &t_temp, &ta);

		// Closes point from the ball to tangent B
		v2 tb = {0};
		col_point_to_line(p, capsule.tangents.b.a, capsule.tangents.b.b, &t_temp, &tb);

		// Get the vector from the ball to the tangents
		v2 tap = v2_sub(p, ta);
		v2 tbp = v2_sub(p, tb);

		f32 tal = v2_len_sq(tap);
		f32 tbl = v2_len_sq(tbp);

		// The closes point from a tangent to the ball
		closest_p = tal < tbl ? ta : tb;
	}

	col_point_to_line(closest_p, capsule.a.p, capsule.b.p, t, closest);
	struct col_cir circle = {
		.p = {closest->x, closest->y},
		.r = lerp(capsule.a.r, capsule.b.r, *t),
	};

	TRACE_END();
	return circle;
}

int
col_circle_to_capsule(struct col_cir a, struct col_capsule b)
{
	TRACE_START(__func__);
	f32 t                   = 0.0f;
	v2 closest              = {0};
	struct col_cir circle_b = col_capsule_get_circle_col(b, a.p, &t, &closest);
	int r                   = col_circle_to_circle(
        a.p.x,
        a.p.y,
        a.r,
        circle_b.p.x,
        circle_b.p.y,
        circle_b.r);

	TRACE_END();
	return r;
}

int
col_circle_to_poly(struct col_cir a, struct col_poly b)
{
	TRACE_START(__func__);
	c2Circle c2a = cir_to_c2cir(a);
	int res      = 0;
	c2Poly c2b   = poly_to_c2poly(b);
	res          = c2CircletoPoly(c2a, &c2b, NULL);
	TRACE_END();
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
	TRACE_START(__func__);
	c2AABB c2a = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2AABB c2b = {.min = {x1b, y1b}, .max = {x2b, y2b}};

	int r = c2AABBtoAABB(c2a, c2b);
	TRACE_END();
	return r;
}

int
col_aabb_to_poly(f32 x1a, f32 y1a, f32 x2a, f32 y2a, struct col_poly b)
{
	// WARN: slow because we do the manifold
	// https://github.com/RandyGaul/cute_headers/issues/404
	TRACE_START(__func__);
	c2AABB c2a   = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	int res      = 0;
	c2Manifold m = {0};
	c2Poly c2b   = poly_to_c2poly(b);
	c2AABBtoPolyManifold(c2a, &c2b, NULL, &m);
	res = m.count > 0;

	TRACE_END();
	return res;
}

struct col_toi
col_circle_toi(struct col_cir a, v2 va, struct col_shape b, v2 vb)
{
	BAD_PATH;
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
		assert(false); // not implemented
	} break;
	case COL_TYPE_CIR: {
		type_b = C2_TYPE_CIRCLE;
		c2_cir = cir_to_c2cir(b.cir);
		c2b    = (void *)&c2_cir;
	} break;
	case COL_TYPE_POLY: {
		NOT_IMPLEMENTED;
	} break;
	default: {
		assert(false);
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
}

void
col_circle_to_circle_manifold(
	f32 ax, f32 ay, f32 ar, f32 bx, f32 by, f32 br, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2Circle c2a   = {.p = {ax, ay}, .r = ar};
	c2Circle c2b   = {.p = {bx, by}, .r = br};
	c2Manifold res = {0};
	c2CircletoCircleManifold(c2a, c2b, &res);
	TRACE_END();
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_aabb_manifold(f32 x, f32 y, f32 r, f32 x1, f32 y1, f32 x2, f32 y2, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2Circle c2a   = {.p = {x, y}, .r = r};
	c2AABB c2b     = {.min = {x1, y1}, .max = {x2, y2}};
	c2Manifold res = {0};
	c2CircletoAABBManifold(c2a, c2b, &res);
	TRACE_END();
	c2manifold_to_manifold(&res, m);
}
void
col_aabb_to_aabb_manifold(f32 x1a, f32 y1a, f32 x2a, f32 y2a, f32 x1b, f32 y1b, f32 x2b, f32 y2b, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2AABB c2a     = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2AABB c2b     = {.min = {x1b, y1b}, .max = {x2b, y2b}};
	c2Manifold res = {0};

	c2AABBtoAABBManifold(c2a, c2b, &res);
	TRACE_END();
	c2manifold_to_manifold(&res, m);
}

void
col_aabb_to_poly_manifold(f32 x1a, f32 y1a, f32 x2a, f32 y2a, struct col_poly b, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2AABB c2a     = {.min = {x1a, y1a}, .max = {x2a, y2a}};
	c2Manifold res = {0};
	c2Poly c2b     = poly_to_c2poly(b);
	c2AABBtoPolyManifold(c2a, &c2b, NULL, &res);
	TRACE_END();
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_capsule_manifold(struct col_cir a, struct col_capsule b, struct col_manifold *m, f32 *t, v2 *closest)
{
	TRACE_START(__func__);
	struct col_cir b_cir = col_capsule_get_circle_col(b, a.p, t, closest);
	struct c2Circle c2b  = cir_to_c2cir(b_cir);
	c2Circle c2a         = cir_to_c2cir(a);
	c2Manifold res       = {0};
	c2CircletoCircleManifold(c2a, c2b, &res);
	TRACE_END();
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_poly_manifold(struct col_cir a, struct col_poly b, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2Circle c2a   = cir_to_c2cir(a);
	c2Manifold c2m = {0};
	c2Poly c2b     = poly_to_c2poly(b);
	c2CircletoPolyManifold(c2a, &c2b, NULL, &c2m);
	TRACE_END();
	c2manifold_to_manifold(&c2m, m);
}

// TODO: optimize division
void
col_point_to_line(v2 c, v2 a, v2 b, f32 *const t, v2 *const d)
{
	TRACE_START(__func__);
	v2 ab = v2_sub(b, a);
	v2 ac = v2_sub(c, a);

	// project c onto ab, computing parametrized position
	*t = v2_dot(ac, ab) / v2_dot(ab, ab);

	// clamp t if outside of segment
	if(*t < 0.0f) *t = 0.0f;
	if(*t > 1.0f) *t = 1.0f;

	// Compute projected position from the clamped t
	*d = v2_add(a, v2_mul(ab, *t));
	TRACE_END();
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

	for(size j = 0; j < poly.count; ++j) {
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
	assert(shape.type != COL_TYPE_NONE);
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
		BAD_PATH;
	}
	}
	return res;
}
