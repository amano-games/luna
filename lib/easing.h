#pragma once

#include "mathfunc.h"

static inline f32 ease_in_sine(f32 t);
static inline f32 ease_out_sine(f32 t);
static inline f32 ease_in_out_sine(f32 t);
static inline f32 ease_in_quad(f32 t);
static inline f32 ease_out_quad(f32 t);
static inline f32 ease_in_out_quad(f32 t);
static inline f32 ease_in_cubic(f32 t);
static inline f32 ease_out_cubic(f32 t);
static inline f32 ease_in_out_cubic(f32 t);
static inline f32 ease_in_quart(f32 t);
static inline f32 ease_out_quart(f32 t);
static inline f32 ease_in_out_quart(f32 t);
static inline f32 ease_in_quint(f32 t);
static inline f32 ease_out_quint(f32 t);
static inline f32 ease_in_out_quint(f32 t);
static inline f32 ease_in_expo(f32 t);
static inline f32 ease_out_expo(f32 t);
static inline f32 ease_in_out_expo(f32 t);
static inline f32 ease_in_circ(f32 t);
static inline f32 ease_out_circ(f32 t);
static inline f32 ease_in_out_circ(f32 t);
static inline f32 ease_in_back(f32 t);
static inline f32 ease_out_back(f32 t);
static inline f32 ease_in_out_back(f32 t);
static inline f32 ease_in_elastic(f32 t);
static inline f32 ease_out_elastic(f32 t);
static inline f32 ease_in_out_elastic(f32 t);
static inline f32 ease_in_bounce(f32 t);
static inline f32 ease_out_bounce(f32 t);
static inline f32 ease_in_out_bounce(f32 t);

static inline f32
ease_in_sine(f32 t)
{
	return 1.0f - cosf((t * PI_FLOAT) / 2.0f);
}

static inline f32
ease_out_sine(f32 t)
{
	return sinf((t * PI_FLOAT) / 2.0f);
}

static inline f32
ease_in_out_sine(f32 t)
{
	return -(cosf(PI_FLOAT * t) - 1.0) / 2.0f;
}

static inline f32
ease_in_quad(f32 t)
{
	return 1.0f - (1.0f - t) * (1.0f - t);
}

static inline f32
ease_out_quad(f32 t)
{
	return 1.0f - (1.0f - t) * (1.0f - t);
}

static inline f32
ease_in_out_quad(f32 t)
{
	return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

static inline f32
ease_in_cubic(f32 t)
{
	return t * t * t;
}

static inline f32
ease_out_cubic(f32 t)
{
	return 1.0f - powf(1.0f - t, 3.0f);
}

static inline f32
ease_in_out_cubic(f32 t)
{
	return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

static inline f32
ease_in_quart(f32 t)
{
	return t * t * t * t;
}

static inline f32
ease_out_quart(f32 t)
{
	return 1.0f - powf(1.0f - t, 4.0f);
}

static inline f32
ease_in_out_quart(f32 t)
{
	return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}

static inline f32
ease_in_quint(f32 t)
{
	return t * t * t * t * t;
}

static inline f32
ease_out_quint(f32 t)
{
	return 1.0f - powf(1.0f - t, 5.0f);
}

static inline f32
ease_in_out_quint(f32 t)
{
	return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}

static inline f32
ease_in_expo(f32 t)
{
	return t == 0 ? 0 : powf(2.0f, 10.0f * t - 10.0f);
}

static inline f32
ease_out_expo(f32 t)
{
	return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}

static inline f32
ease_in_out_expo(f32 t)
{
	return t == 0
		? 0
		: t == 1.0f
		? 1.0f
		: t < 0.5f ? powf(2.0f, 20.0f * t - 10.0f) / 2.0f
				   : (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

static inline f32
ease_in_circ(f32 t)
{
	return 1.0f - sqrtf(1.0f - powf(t, 2.0f));
}

static inline f32
ease_out_circ(f32 t)
{
	return sqrtf(1.0f - powf(t - 1, 2.0f));
}

static inline f32
ease_in_out_circ(f32 t)
{
	return t < 0.5f
		? (1.0f - sqrtf(1.0f - powf(2.0f * t, 2.0f))) / 2.0f
		: (sqrtf(1.0f - powf(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

static inline f32
ease_in_back(f32 t)
{
	f32 c1 = 1.70158f;
	f32 c3 = c1 + 1.0f;

	return c3 * t * t * t - c1 * t * t;
}

static inline f32
ease_out_back(f32 t)
{
	f32 c1 = 1.70158f;
	f32 c3 = c1 + 1.0f;

	return 1 + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

static inline f32
ease_in_out_back(f32 t)
{
	f32 c1 = 1.70158f;
	f32 c2 = c1 * 1.525f;

	return t < 0.5
		? (powf(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
		: (powf(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

static inline f32
ease_in_elastic(f32 t)
{
	f32 c4 = (2.0f * PI_FLOAT) / 3;

	return t == 0
		? 0
		: t == 1.0f
		? 1.0f
		: -powf(2.0f, 10.0f * t - 10.0f) * sin_f((t * 10.0f - 10.75f) * c4);
}

static inline f32
ease_out_elastic(f32 t)
{
	f32 c4 = (2.0f * PI_FLOAT) / 3;

	return t == 0
		? 0
		: t == 1
		? 1.0f
		: powf(2.0f, -10.0f * t) * sin_f((t * 10.0f - 0.75f) * c4) + 1.0f;
}

static inline f32
ease_in_out_elastic(f32 t)
{
	f32 c5 = (2.0f * PI_FLOAT) / 4.5f;

	return t == 0
		? 0
		: t == 1
		? 1.0f
		: t < 0.5f
		? -(powf(2.0f, 20.0f * t - 10.0f) * sin_f((20.0f * t - 11.125f) * c5)) / 2.0f
		: (powf(2.0f, -20.0f * t + 10.0f) * sin_f((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}

static inline f32
ease_in_bounce(f32 t)
{
	return 1.0f - ease_out_bounce(1.0f - t);
}

static inline f32
ease_out_bounce(f32 t)
{
	f32 n1 = 7.5625f;
	f32 d1 = 2.75f;

	if(t < 1.0f / d1) {
		return n1 * t * t;
	} else if(t < 2.0f / d1) {
		t -= 1.5f;
		return n1 * (t / d1) * t + 0.75f;
	} else if(t < 2.5f / d1) {
		t -= 2.25f;
		return n1 * (t / d1) * t + 0.9375f;
	} else {
		t -= 2.625f;
		return n1 * (t / d1) * t + 0.984375f;
	}
}

static inline f32
ease_in_out_bounce(f32 t)
{
	return t < 0.5f
		? (1.0f - ease_out_bounce(1.0f - 2.0f * t)) / 2.0f
		: (1.0f + ease_out_bounce(2.0f * t - 1.0f)) / 2.0f;
}
