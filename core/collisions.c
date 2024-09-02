#include "collisions.h"

#include "sys-assert.h"
#include "sys-types.h"

#include "trace.h"
#include "mathfunc.h"
#include "v2.h"

void
col_poly_init(struct col_poly *p)
{
	TRACE_START(__func__);

	for(usize i = 0; i < p->count; ++i) {
		c2Poly c2p = poly_to_c2poly(p->sub_polys[i]);
		assert(p->sub_polys[i].count <= COL_MAX_POLYGON_VERTS);
		c2MakePoly(&c2p);

		struct sub_poly *sub_poly = &p->sub_polys[i];
		sub_poly->count           = c2p.count;

		for(int j = 0; j < c2p.count; ++j) {
			sub_poly->verts[j] = c2v_to_v2(c2p.verts[j]);
			sub_poly->norms[j] = c2v_to_v2(c2p.norms[j]);
		}
	}

	TRACE_END();
}

// https://stackoverflow.com/questions/2792443/finding-the-centroid-of-a-polygon
v2
col_poly_centroid(struct col_poly *p)
{
	TRACE_START(__func__);
	v2 centroid = {0.0, 0.0};

	f32 signed_area = 0.0f;
	f32 x0          = 0.0f; // current vertex X
	f32 y0          = 0.0f; // current vertex Y
	f32 x1          = 0.0f; // Next vertex X
	f32 y1          = 0.0f; // next vertex Y
	f32 a           = 0.0f; // partial signed area

	usize vertex_count = 0;
	// TODO: memory allocate ?
	v2 vertices[COL_MAX_POLYGON_VERTS * COL_MAX_SUB_POLY] = {0};
	for(usize i = 0; i < p->count; ++i) {
		struct sub_poly sub_poly = p->sub_polys[i];
		for(int j = 0; j < sub_poly.count; ++j) {
			vertices[vertex_count++] = sub_poly.verts[j];
		}
	}

	{
		f32 signed_area = 0.0;
		f32 x0          = 0.0; // Current vertex X
		f32 y0          = 0.0; // Current vertex Y
		f32 x1          = 0.0; // Next vertex X
		f32 y1          = 0.0; // Next vertex Y
		f32 a           = 0.0; // Partial signed area

		int lastdex = vertex_count - 1;
		v2 *prev    = &(vertices[lastdex]);
		v2 *next;

		// For all vertices in a loop
		for(usize i = 0; i < vertex_count; ++i) {
			next = &(vertices[i]);
			x0   = prev->x;
			y0   = prev->y;
			x1   = next->x;
			y1   = next->y;
			a    = x0 * y1 - x1 * y0;
			signed_area += a;
			centroid.x += (x0 + x1) * a;
			centroid.y += (y0 + y1) * a;
			prev = next;
		}

		signed_area *= 0.5f;
		centroid.x /= (6.0f * signed_area);
		centroid.y /= (6.0f * signed_area);
	}

	TRACE_END();
	return centroid;
}

struct col_cir
col_merge_circles(struct col_cir a, struct col_cir b)
{
	TRACE_START(__func__);
	struct col_cir res = {0};
	v2 d               = v2_sub(b.p, a.p);
	f32 dist2          = v2_dot(d, d);

	if(sqrt_f32(b.r - a.r) >= dist2) {
		// The shere with the larger radius encloses the other;
		// just set s to be the larget of the two spheres
		if(b.r >= a.r) {
			res = b;
		} else {
			res = a;
		}
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
col_circle_to_circle(struct col_cir a, struct col_cir b)
{
	TRACE_START(__func__);
	c2Circle c2a = cir_to_c2cir(a);
	c2Circle c2b = cir_to_c2cir(b);
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

	col_point_to_line(closest_p, capsule.a, capsule.b, t, closest);
	struct col_cir circle = {
		.p = {closest->x, closest->y},
		.r = lerp(capsule.ra, capsule.rb, *t),
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
	int r                   = col_circle_to_circle(a, circle_b);
	TRACE_END();
	return r;
}

int
col_circle_to_poly(struct col_cir a, struct col_poly b)
{
	TRACE_START(__func__);
	c2Circle c2a = cir_to_c2cir(a);
	int r        = 0;
	for(usize i = 0; i < b.count; ++i) {
		c2Poly c2b = poly_to_c2poly(b.sub_polys[i]);
		r          = c2CircletoPoly(c2a, &c2b, NULL);
		if(r) {
			break;
		}
	}

	TRACE_END();
	return r;
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
col_circle_to_circle_manifold(struct col_cir a, struct col_cir b, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2Circle c2a   = cir_to_c2cir(a);
	c2Circle c2b   = cir_to_c2cir(b);
	c2Manifold res = {0};
	c2CircletoCircleManifold(c2a, c2b, &res);
	TRACE_END();
	c2manifold_to_manifold(&res, m);
}

void
col_circle_to_aabb_manifold(struct col_cir a, struct col_aabb b, struct col_manifold *m)
{
	TRACE_START(__func__);
	c2Circle c2a   = cir_to_c2cir(a);
	c2AABB c2b     = aabb_to_c2aabb(b);
	c2Manifold res = {0};
	c2CircletoAABBManifold(c2a, c2b, &res);
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

	for(usize i = 0; i < b.count; ++i) {
		struct sub_poly sub_poly = b.sub_polys[i];
		c2Poly c2b               = poly_to_c2poly(sub_poly);

		c2CircletoPolyManifold(c2a, &c2b, NULL, &c2m);
		if(c2m.count > 0) {
			break;
		}
	}
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
