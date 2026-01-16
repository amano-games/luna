#pragma once

#include "base/types.h"
#include "base/v2.h"
#include "engine/collisions/collisions.h"

v2
capsule_get_end(f32 ax, f32 ay, f32 len, rot2 rot)
{
	v2 res = {ax, ay + len};
	res    = v2_rot_anchor(res, rot, (v2){ax, ay});
	return res;
}

struct col_tangents
capsule_get_tangents_v2(
	f32 ax,
	f32 ay,
	f32 ra,
	f32 bx,
	f32 by,
	f32 rb,
	f32 ign)
{
	struct col_tangents res = {0};
	v2 a                    = {ax, ay};
	v2 b                    = {bx, by};

	v2 ab   = v2_sub(b, a);
	f32 len = v2_len(ab);
	if(len < EPSILON) {
		return res;
	}
	dbg_assert(abs_f32(ra - rb) <= len);

	f32 len_inv = 1.0f / len;
	// Axis direction
	v2 dir = v2_mul(ab, 1.0f * len_inv);

	// Perpendicular normal
	v2 n = {-dir.y, dir.x};

	// Radius slope
	f32 dr    = rb - ra;
	f32 slope = dr * len_inv;

	f32 k2 = 1.0f - slope * slope;
	if(k2 < 0.0f) {
		return res;
	}

	f32 h = sqrt_f32(k2);

	// Two directions
	v2 nh        = v2_mul(n, h);
	v2 dir_slope = v2_mul(dir, slope);
	v2 n1        = v2_add(v2_mul(n, h), dir_slope);  // one side
	v2 n2        = v2_add(v2_mul(n, -h), dir_slope); // opposite side

	// Side segments
	res.a.a = v2_add(a, v2_mul(n1, ra));
	res.a.b = v2_add(b, v2_mul(n1, rb));

	res.b.a = v2_add(a, v2_mul(n2, ra));
	res.b.b = v2_add(b, v2_mul(n2, rb));

	return res;
}

struct col_tangents
capsule_get_tangents(f32 ax, f32 ay, f32 ra, f32 bx, f32 by, f32 rb, f32 len)
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
	f32 len = capsule->d;
	v2 a    = capsule->a.p;
	v2 b    = capsule->b.p;
	// f32 len           = v2_len(v2_sub(b, a));
	capsule->b.p      = capsule_get_end(a.x, a.y, len, rot);
	capsule->tangents = capsule_get_tangents(a.x, a.y, capsule->a.r, capsule->b.p.x, capsule->b.p.y, capsule->b.r, capsule->d);
}
