#pragma once

#include "collisions.h"
#include "sys-types.h"
#include "v2.h"

v2
capsule_get_end(f32 ax, f32 ay, f32 len, rot2 rot)
{
	v2 res = {ax, ay + len};
	res    = v2_rot_anchor(res, rot, (v2){ax, ay});
	return res;
}

struct col_tangents
capsule_get_tangents(f32 ax, f32 ay, f32 bx, f32 by, f32 ra, f32 rb, f32 len)
{
	f32 dx = bx - ax;
	f32 dy = by - ay;

	// if (dist <= Math.abs(r2 - r1)) return; // no valid tangents

	// Rotation from x-axis
	// Maybe angle1 is the angle between ends
	f32 angle_1 = atan2_f32(dy, dx);
	f32 angle_2 = acos_f32((ra - rb) / len);

	f32 angle_a = angle_1 + angle_2;
	f32 angle_b = angle_1 - angle_2;

	rot2 rot_a = {
		.c = cos_f32(angle_a),
		.s = sin_f32(angle_a),
	};

	rot2 rot_b = {
		.c = cos_f32(angle_b),
		.s = sin_f32(angle_b),
	};

	struct col_tangents res = {
		.a = {
			.a = {ax + ra * rot_a.c, ay + ra * rot_a.s},
			.b = {bx + rb * rot_a.c, by + rb * rot_a.s},
		},
		.b = {
			.a = {ax + ra * rot_b.c, ay + ra * rot_b.s},
			.b = {bx + rb * rot_b.c, by + rb * rot_b.s},
		},
	};

	return res;
}

void
capsule_upd(struct col_capsule *capsule, rot2 rot)
{
	f32 len           = capsule->d;
	v2 a              = capsule->a;
	capsule->b        = capsule_get_end(a.x, a.y, len, rot);
	capsule->tangents = capsule_get_tangents(a.x, a.y, capsule->b.x, capsule->b.y, capsule->ra, capsule->rb, len);
}
