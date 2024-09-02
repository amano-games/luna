#pragma once

#include "trace.h"
#include "sys-types.h"
#include "mathfunc.h"

static inline v2
v2_add(v2 a, v2 b)
{
	v2 r = {a.x + b.x, a.y + b.y};
	return r;
}

static inline v2_i32
v2_add_i32(v2_i32 a, v2_i32 b)
{
	v2_i32 r = {a.x + b.x, a.y + b.y};
	return r;
}

static inline v2
v2_sub(v2 a, v2 b)
{
	v2 r = {a.x - b.x, a.y - b.y};
	return r;
}

static inline v2_i32
v2_sub_i32(v2_i32 a, v2_i32 b)
{
	v2_i32 r = {a.x - b.x, a.y - b.y};
	return r;
}

static inline v2
v2_mul(v2 v, f32 s)
{
	v2 r = {v.x * s, v.y * s};
	return r;
}

static inline v2
v2_div(v2 v, f32 s)
{
	v2 r = {v.x / s, v.y / s};
	return r;
}

static inline f32
v2_dot(v2 a, v2 b)
{
	return a.x * b.x + a.y * b.y;
}

static inline v2
v2_crs(float s, v2 v)
{
	v2 r = {-s * v.y, s * v.x};
	return r;
}

static inline f32
v2_crs_v2(v2 a, v2 b)
{
	return a.x * b.y - a.y * b.x;
}

static inline v2
v2_min(v2 a, v2 b)
{
	v2 r = {min_f32(a.x, b.x), min_f32(a.y, b.y)};
	return r;
}

static inline v2
v2_max(v2 a, v2 b)
{
	v2 r = {max_f32(a.x, b.x), max_f32(a.y, b.y)};
	return r;
}

static inline v2
v2_clamp(v2 a, v2 lo, v2 hi)
{
	v2 r = v2_max(lo, v2_min(a, hi));
	return r;
}

static inline v2
v2_abs(v2 a)
{
	v2 r = {abs_f32(a.x), abs_f32(a.y)};
	return r;
}

static inline f32
v2_len_sq(v2 a)
{
	f32 x = abs_f32(a.x);
	f32 y = abs_f32(a.y);
	return x * x + y * y;
}

static inline f32
v2_len(v2 v)
{
	return sqrt_f32(v2_len_sq(v));
}

static inline v2
v2_normalized(v2 a)
{
	return v2_div(a, v2_len(a));
}

static inline v2
v2_normalized_safe(v2 a)
{
	f32 len, len_inv;
	len = sqrtf(a.x * a.x + a.y * a.y);

	if(len == 0) len = 1.0f;

	len_inv = 1.0f / len;

	v2 r = {a.x * len_inv, a.y * len_inv};

	return r;
}

static inline v2
v2_clamp_mag(v2 v, f32 lo, f32 hi)
{
	f32 mag = v2_len(v);

	if(mag < lo && mag > EPSILON) {
		v2 normalized = v2_div(v, mag);
		normalized.x *= lo;
		normalized.y *= lo;
		return normalized;
	} else if(mag > hi) {
		v2 normalized = v2_div(v, mag);
		normalized.x *= hi;
		normalized.y *= hi;
		return normalized;
	} else {
		return v;
	}
}

v2
v2_clamp_mag_sq(v2 v, float lo_sq, float hi_sq)
{
	float mag_sq = v2_len_sq(v);

	if(mag_sq < lo_sq) {
		v = v2_div(v, mag_sq);
		v.x *= lo_sq;
		v.y *= lo_sq;
	} else if(mag_sq > hi_sq) {
		v = v2_div(v, mag_sq);
		v.x *= hi_sq;
		v.y *= hi_sq;
	}

	return v;
}

static inline v2
v2_normal(v2 a)
{
	return v2_normalized((v2){a.y, -a.x});
}

static inline u32
v2_distance_sq(v2 a, v2 b)
{
	return v2_len_sq(v2_sub(a, b));
}

static inline v2
v2_rot(v2 v, f32 a)
{
	f32 angle = a;
	v2 r      = {
			 .x = v.x * cosf(angle) - v.y * sinf(angle),
			 .y = v.x * sinf(angle) + v.y * cosf(angle)};
	return r;
}

static inline v2
v2_rot_anchor(v2 v, rot2 rot, v2 p)
{
	TRACE_START(__func__);
	f32 cos_theta = rot.c;
	f32 sin_theta = rot.s;

	f32 x = (cos_theta * (v.x - p.x) - sin_theta * (v.y - p.y) + p.x);
	f32 y = (sin_theta * (v.x - p.x) + cos_theta * (v.y - p.y) + p.y);

	v2 r = {.x = x, .y = y};
	TRACE_END();

	return r;
}

static inline v2_i32
v2_round(v2 a)
{
	v2_i32 v = {(i32)(a.x + .5f), (i32)(a.y + .5f)};
	return v;
}

static inline v2
v2_lerp(v2 a, v2 b, f32 t)
{
	v2 v = {
		.x = lerp(a.x, b.x, t),
		.y = lerp(a.y, b.y, t),
	};
	return v;
}

static inline v2
v2_move_towards(v2 a, v2 b, f32 delta)
{
	v2 ba   = v2_sub(b, a);
	f32 len = v2_len(ba);

	if(len <= delta || len < EPSILON) {
		return b;
	} else {
		f32 len_inv = 1.0f / len;
		v2 norm     = {ba.x * len_inv, a.y * len_inv};
		v2 res      = v2_add(a, v2_mul(norm, delta));
		return res;
	}
}
