#pragma once
#include "base/types.h"
#include "mathfunc.h"

static inline mat22
mat2_transpose(mat22 m)
{
	return (mat22){m.m00, m.m10, m.m01, m.m11};
}

static inline f32
mat2_dot(mat22 a, mat22 b)
{
	f32 sum = 0.0f;

	sum += a.m00 * b.m00;
	sum += a.m01 * b.m01;
	sum += a.m10 * b.m10;
	sum += a.m11 * b.m11;

	return sum;
}

static inline mat22
mat2_scale(mat22 m, f32 s)
{
	mat22 r = {
		.m00 = m.m00 * s,
		.m01 = m.m01 * s,
		.m10 = m.m10 * s,
		.m11 = m.m11 * s,
	};

	return r;
}

static inline mat22
mat2_add(mat22 a, mat22 b)
{
	mat22 r = {
		.m00 = a.m00 + b.m00,
		.m01 = a.m01 + b.m01,
		.m10 = a.m10 + b.m10,
		.m11 = a.m11 + b.m11};

	return r;
}

static inline mat22
mat2_sub(mat22 a, mat22 b)
{
	mat22 r = {
		.m00 = a.m00 - b.m00,
		.m01 = a.m01 - b.m01,
		.m10 = a.m10 - b.m10,
		.m11 = a.m11 - b.m11};

	return r;
}

static inline v2
mat2_multiply_v2(mat22 m, v2 v)
{
	return (v2){m.m00 * v.x + m.m01 * v.y, m.m10 * v.x + m.m11 * v.y};
}
