#include "physics.h"

#include "lib/poly.h"
#include "base/types.h"

#include "base/mathfunc.h"
#include "base/dbg.h"
#include "base/v2.h"

void
body_init(struct body *body)
{
	for(size i = 0; i < body->shapes.count; ++i) {
		struct col_shape *shape = body->shapes.items + i;
		switch(shape->type) {
		case COL_TYPE_POLY: {
			body->p = poly_centroid(shape->poly.verts, shape->poly.count);
		} break;
		case COL_TYPE_CIR: {
			f32 r         = shape->cir.r;
			body->inertia = (0.5f * (r * r)) * body->mass;
		} break;
		case COL_TYPE_AABB:
			break;
		case COL_TYPE_CAPSULE:
			break;
		default:
			dbg_sentinel("body");
		}
	}

	if(body->mass != 0.0f) {
		body->mass_inv = 1.0f / body->mass;
	} else {
		body->mass_inv = 0.0f;
	}

	if(body->inertia != 0.0f) {
		body->inertia_inv = 1.0f / body->inertia;
	} else {
		body->inertia_inv = 0.0f;
	}

error:
	return;
}

void
body_reset(struct body *body, f32 x, f32 y)
{
	body->flags         = 0;
	body->vel.x         = 0;
	body->vel.y         = 0;
	body->vel_delta.x   = 0;
	body->vel_delta.y   = 0;
	body->ang_vel       = 0;
	body->ang_vel_delta = 0;
	body->p_delta.x     = 0;
	body->p_delta.y     = 0;
	body->p.x           = x;
	body->p.y           = y;
}

void
body_add_force(struct body *body, const v2 force)
{
	body->force = v2_add(body->force, force);
}

void
body_clear_forces(struct body *body)
{
	body->force     = (v2){0.0f, 0.0f};
	body->vel_delta = (v2){0.0f, 0.0f};
}

void
body_apply_linear_impulse(struct body *body, v2 j)
{
	body->vel_delta.x += body->mass_inv * j.x;
	body->vel_delta.y += body->mass_inv * j.y;
}

void
body_apply_angular_impulse(struct body *body, v2 j, v2 r)
{
	body_apply_linear_impulse(body, j);

	f32 crs = v2_crs_v2(r, j);
	body->ang_vel_delta += body->inertia_inv * crs;
}

void
body_add_torque(struct body *body, const f32 torque)
{
	body->torque += torque;
}

void
body_clear_torque(struct body *body)
{
	body->torque        = 0.0f;
	body->ang_vel_delta = 0.0f;
}

v2
body_gravity_force(struct body *body, f32 gravity)
{
	return (v2){0.0, body->mass * gravity};
}

void
body_integrate_linear(struct physics *physics, struct body *body)
{
	// TODO: check null ?
	f32 dt                         = physics->dt;
	f32 dt_inv                     = physics->dt_inv;
	f32 max_translation            = physics->max_translation;
	f32 damping                    = 1.0f - body->linear_damping;
	f32 max_linear_speed           = max_translation * dt_inv;
	float max_linear_speed_squared = max_linear_speed * max_linear_speed;

	v2 acc          = v2_mul(body->force, body->mass_inv);
	body->vel_delta = v2_add(body->vel_delta, v2_mul(acc, dt));

	v2 vel_damp = v2_mul(body->vel, damping);
	body->vel   = v2_add(vel_damp, body->vel_delta);

	if(v2_dot(body->vel, body->vel) > max_linear_speed_squared) {
		f32 ratio = max_linear_speed / v2_len(body->vel);
		body->vel = v2_mul(body->vel, ratio);
	}

	body_clear_forces(body);
}

void
body_integrate_angular(struct physics *physics, struct body *body)
{
	f32 dt                    = physics->dt;
	f32 dt_inv                = physics->dt_inv;
	f32 max_rotation          = physics->max_rotation;
	f32 w                     = body->ang_vel;
	f32 max_ang_speed         = max_rotation * dt_inv;
	f32 max_ang_speed_squared = max_ang_speed * max_ang_speed;
	f32 damping               = 1.0f - body->ang_damping;

	f32 acc = body->torque * body->inertia_inv;
	body->ang_vel_delta += acc * dt;
	w = body->ang_vel_delta + (damping * w);

	// Clamp to max angular speed
	if(w * w > max_ang_speed_squared) {
		float ratio = max_ang_speed / abs_f32(w);
		w *= ratio;
	}

	body->ang_vel = w;
	body_clear_torque(body);
}

void
body_integrate(struct physics *physics, struct body *body)
{
	body_integrate_linear(physics, body);
	body_integrate_angular(physics, body);

	body->p_delta = v2_add(body->p_delta, body->vel);
	body->p       = v2_add(body->p, body->p_delta);

	body->orient = body->orient + body->ang_vel;

	body->p_delta = (v2){0.0f, 0.0f};
}

void
body_positional_correction(struct physics *physics, struct body *a, struct body *b, struct col_manifold *m)
{
	f32 penetration    = m->depth;
	f32 percent        = physics->penetration_correction;
	f32 slop           = physics->penetration_allowance;
	f32 correction_mag = max_f32(penetration - slop, 0.0f) / (a->mass_inv + b->mass_inv);
	v2 correction      = v2_mul(m->normal, correction_mag * percent);

	a->p_delta = v2_mul(correction, -a->mass_inv);
	b->p_delta = v2_mul(correction, b->mass_inv);
}

void
body_impulse_correction(
	struct body *a,
	struct body *b,
	f32 restitution_slop,
	struct col_manifold *m)
{
	/// Restitution mixing law. The idea is allow for anything to bounce off an inelastic surface.
	/// For example, a superball bounces on anything.
	f32 restitution = max_f32(a->restitution, b->restitution);

	v2 start = m->contact_points[0];
	v2 end   = v2_add(v2_mul(v2_normalized(m->normal), m->depth), start);

	v2 ra = v2_sub(end, a->p);
	v2 rb = v2_sub(start, b->p);

	v2 va = v2_add(a->vel, (v2){-a->ang_vel * ra.y, a->ang_vel * ra.x});
	v2 vb = v2_add(b->vel, (v2){-b->ang_vel * rb.y, b->ang_vel * rb.x});

	v2 rv = v2_sub(va, vb);

	// TODO: move this to a manifold generation ?
	// Determine if we should perform a resting collision or not
	// The idea is if the only thing moving this object is gravity,
	// then the collision should be performed without any restitution
	if(v2_len_sq(rv) < (restitution_slop * restitution_slop) + EPSILON) {
		restitution = 0.0f;
	}

	float vrel_dot_n   = v2_dot(rv, m->normal);
	f32 ra_crs_n       = v2_crs_v2(ra, m->normal);
	f32 rb_crs_n       = v2_crs_v2(rb, m->normal);
	f32 ra_crs_n2      = ra_crs_n * ra_crs_n;
	f32 rb_crs_n2      = rb_crs_n * rb_crs_n;
	f32 mass_inv_sum_n = (a->mass_inv + b->mass_inv) + ra_crs_n2 * a->inertia_inv + rb_crs_n2 * b->inertia_inv;

	v2 jn_dir  = m->normal;
	f32 jn_mag = 0.0f;
	v2 jn      = {0};
	// Only apply the impulse if bodies are approaching
	if(vrel_dot_n > 0.0f) {
		jn_mag = -(1.0f + restitution) * vrel_dot_n / mass_inv_sum_n;
		jn     = v2_mul(jn_dir, jn_mag);
	}

	body_apply_angular_impulse(a, jn, ra);
	body_apply_angular_impulse(b, v2_mul(jn, -1.0f), rb);

	// TODO: get rid of the sqrtf
	f32 dynamic_friction = sqrt_f32(a->dynamic_friction * b->dynamic_friction);
	f32 static_friction  = sqrt_f32(a->static_friction * b->static_friction);

	v2 tangent = v2_normal(m->normal);

	f32 ra_crs_t       = v2_crs_v2(ra, tangent);
	f32 rb_crs_t       = v2_crs_v2(rb, tangent);
	f32 ra_crs_t2      = ra_crs_t * ra_crs_t;
	f32 rb_crs_t2      = rb_crs_t * rb_crs_t;
	f32 mass_inv_sum_t = (a->mass_inv + b->mass_inv) * ra_crs_t2 * a->inertia_inv + rb_crs_t2 * b->inertia_inv;
	f32 vrel_dot_t     = v2_dot(rv, tangent);

	v2 jt_dir  = tangent;
	f32 jt_mag = -(1 + restitution) * vrel_dot_t / mass_inv_sum_t;

	if(abs_f32(jt_mag) >= jn_mag * static_friction) {
		jt_mag = jt_mag * dynamic_friction;
	}

	if(f32_equal(jt_mag, 0.0f)) {
		return;
	}

	v2 jt = v2_mul(jt_dir, jt_mag);

	body_apply_angular_impulse(a, jt, ra);
	body_apply_angular_impulse(b, v2_mul(jt, -1.0f), rb);
}
