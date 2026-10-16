#pragma once

#include "base/types.h"
#include "base/mathfunc.h"

static inline v3
v3_sub(v3 a, v3 b)
{
	v3 r = {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
	return r;
}

static inline f32
v3_len_sq(v3 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline f32
v3_len(v3 v)
{
	return sqrt_f32(v3_len_sq(v));
}

static inline f32
v3_distance_sq(v3 a, v3 b)
{
	return v3_len_sq(v3_sub(a, b));
}

static inline f32
v3_distance(v3 a, v3 b)
{
	return sqrt_f32(v3_distance_sq(a, b));
}
