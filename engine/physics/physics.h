#pragma once

// https://github.com/victorfisac/Physac/tree/master

#include "base/types.h"
#include "engine/collisions/collisions.h"

struct physics {
	u8 steps;
	f32 steps_inv;
	f32 dt;
	f32 dt_inv;
	f32 max_translation;
	f32 max_rotation;
	f32 penetration_correction;
	f32 penetration_allowance;
};

struct body {
	u32 flags;

	v2 p;         // position
	v2 p_delta;   // position correction
	v2 vel;       // velocity
	v2 vel_delta; // velocity delta

	f32 restitution;      // bouncines
	f32 dynamic_friction; // friction for angular velocity
	f32 static_friction;  // friction for angular velocity

	f32 mass;
	f32 mass_inv;

	f32 orient;
	f32 ang_vel;
	f32 ang_vel_delta;

	f32 inertia;     // moment of inertia
	f32 inertia_inv; // inverse moment of inertia

	v2 force;
	f32 linear_damping;
	f32 torque;
	f32 ang_damping;

	struct col_shapes shapes;
};

void body_init(struct body *body);

void body_integrate(struct physics *physics, struct body *body);
void body_integrate_linear(struct physics *physics, struct body *body);
void body_integrate_angular(struct physics *physics, struct body *body);

void body_add_force(struct body *body, const v2 force);
void body_clear_forces(struct body *body);
void body_reset(struct body *body, f32 x, f32 y);

void body_apply_linear_impulse(struct body *body, v2 j);
void body_apply_angular_impulse(struct body *body, v2 j, v2 r);

void body_add_torque(struct body *body, const f32 torque);
void body_clear_torque(struct body *body);

v2 body_gravity_force(struct body *body, f32 gravity);

void body_positional_correction(struct physics *physics, struct body *a, struct body *b, struct col_manifold *m);
void body_impulse_correction(struct body *a, struct body *b, f32 restitution_slop, struct col_manifold *m);
