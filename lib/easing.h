#pragma once

#include "easing-type.h"
#include "mathfunc.h"

static inline f32 ease_sine_in(f32 t);
static inline f32 ease_sine_out(f32 t);
static inline f32 ease_sine_in_out(f32 t);
static inline f32 ease_quad_in(f32 t);
static inline f32 ease_quad_out(f32 t);
static inline f32 ease_quad_in_out(f32 t);
static inline f32 ease_cubic_in(f32 t);
static inline f32 ease_cubic_out(f32 t);
static inline f32 ease_cubic_in_out(f32 t);
static inline f32 ease_quart_in(f32 t);
static inline f32 ease_quart_out(f32 t);
static inline f32 ease_quart_in_out(f32 t);
static inline f32 ease_quint_in(f32 t);
static inline f32 ease_quint_out(f32 t);
static inline f32 ease_quint_in_out(f32 t);
static inline f32 ease_expo_in(f32 t);
static inline f32 ease_expo_out(f32 t);
static inline f32 ease_expo_in_out(f32 t);
static inline f32 ease_circ_in(f32 t);
static inline f32 ease_circ_out(f32 t);
static inline f32 ease_circ_in_out(f32 t);
static inline f32 ease_back_in(f32 t);
static inline f32 ease_back_out(f32 t);
static inline f32 ease_back_in_out(f32 t);
static inline f32 ease_elastic_in(f32 t);
static inline f32 ease_elastic_out(f32 t);
static inline f32 ease_elastic_in_out(f32 t);
static inline f32 ease_bounce_in(f32 t);
static inline f32 ease_bounce_out(f32 t);
static inline f32 ease_bounce_in_out(f32 t);

static inline f32 ease(f32 t, enum ease_type type);

static inline f32
ease_sine_in(f32 t)
{
	return 1.0f - cos_f32((t * PI_FLOAT) / 2.0f);
}

static inline f32
ease_sine_out(f32 t)
{
	return sin_f32((t * PI_FLOAT) / 2.0f);
}

static inline f32
ease_sine_in_out(f32 t)
{
	return -(cos_f32(PI_FLOAT * t) - 1.0f) / 2.0f;
}

static inline f32
ease_quad_in(f32 t)
{
	return 1.0f - (1.0f - t) * (1.0f - t);
}

static inline f32
ease_quad_out(f32 t)
{
	return 1.0f - (1.0f - t) * (1.0f - t);
}

static inline f32
ease_quad_in_out(f32 t)
{
	return t < 0.5f ? 2.0f * t * t : 1.0f - pow_f32(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

static inline f32
ease_cubic_in(f32 t)
{
	return t * t * t;
}

static inline f32
ease_cubic_out(f32 t)
{
	return 1.0f - pow_f32(1.0f - t, 3.0f);
}

static inline f32
ease_cubic_in_out(f32 t)
{
	return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow_f32(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

static inline f32
ease_quart_in(f32 t)
{
	return t * t * t * t;
}

static inline f32
ease_quart_out(f32 t)
{
	return 1.0f - pow_f32(1.0f - t, 4.0f);
}

static inline f32
ease_quart_in_out(f32 t)
{
	return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - pow_f32(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}

static inline f32
ease_quint_in(f32 t)
{
	return t * t * t * t * t;
}

static inline f32
ease_quint_out(f32 t)
{
	return 1.0f - pow_f32(1.0f - t, 5.0f);
}

static inline f32
ease_quint_in_out(f32 t)
{
	return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - pow_f32(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}

static inline f32
ease_expo_in(f32 t)
{
	return t == 0 ? 0 : pow_f32(2.0f, 10.0f * t - 10.0f);
}

static inline f32
ease_expo_out(f32 t)
{
	f32 p = pow_f32(2.0f, -10.0f * t);
	return t == 1.0f ? 1.0f : 1.0f - p;
}

static inline f32
ease_expo_in_out(f32 t)
{
	return t == 0
		? 0
		: t == 1.0f
		? 1.0f
		: t < 0.5f ? pow_f32(2.0f, 20.0f * t - 10.0f) / 2.0f
				   : (2.0f - pow_f32(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

static inline f32
ease_circ_in(f32 t)
{
	return 1.0f - sqrtf(1.0f - pow_f32(t, 2.0f));
}

static inline f32
ease_circ_out(f32 t)
{
	return sqrtf(1.0f - pow_f32(t - 1, 2.0f));
}

static inline f32
ease_circ_in_out(f32 t)
{
	return t < 0.5f
		? (1.0f - sqrtf(1.0f - pow_f32(2.0f * t, 2.0f))) / 2.0f
		: (sqrtf(1.0f - pow_f32(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

static inline f32
ease_back_in(f32 t)
{
	f32 c1 = 1.70158f;
	f32 c3 = c1 + 1.0f;

	return c3 * t * t * t - c1 * t * t;
}

static inline f32
ease_back_out(f32 t)
{
	f32 c1 = 1.70158f;
	f32 c3 = c1 + 1.0f;

	return 1 + c3 * pow_f32(t - 1.0f, 3.0f) + c1 * pow_f32(t - 1.0f, 2.0f);
}

static inline f32
ease_back_in_out(f32 t)
{
	f32 c1 = 1.70158f;
	f32 c2 = c1 * 1.525f;

	return t < 0.5f
		? (pow_f32(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
		: (pow_f32(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

static inline f32
ease_elastic_in(f32 t)
{
	f32 c4 = (2.0f * PI_FLOAT) / 3;

	return t == 0
		? 0
		: t == 1.0f
		? 1.0f
		: -pow_f32(2.0f, 10.0f * t - 10.0f) * sin_f32((t * 10.0f - 10.75f) * c4);
}

static inline f32
ease_elastic_out(f32 t)
{
	f32 c4 = (2.0f * PI_FLOAT) / 3;

	return t == 0
		? 0
		: t == 1
		? 1.0f
		: pow_f32(2.0f, -10.0f * t) * sin_f32((t * 10.0f - 0.75f) * c4) + 1.0f;
}

static inline f32
ease_elastic_in_out(f32 t)
{
	f32 c5 = (2.0f * PI_FLOAT) / 4.5f;

	return t == 0
		? 0
		: t == 1
		? 1.0f
		: t < 0.5f
		? -(pow_f32(2.0f, 20.0f * t - 10.0f) * sin_f32((20.0f * t - 11.125f) * c5)) / 2.0f
		: (pow_f32(2.0f, -20.0f * t + 10.0f) * sin_f32((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}

static inline f32
ease_bounce_in(f32 t)
{
	return 1.0f - ease_bounce_out(1.0f - t);
}

static inline f32
ease_bounce_out(f32 t)
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
ease_bounce_in_out(f32 t)
{
	return t < 0.5f
		? (1.0f - ease_bounce_out(1.0f - 2.0f * t)) / 2.0f
		: (1.0f + ease_bounce_out(2.0f * t - 1.0f)) / 2.0f;
}

static inline f32
ease(f32 t, enum ease_type type)
{
	switch(type) {
	case EASE_TYPE_SINE_IN: {
		return ease_sine_in(t);
	} break;
	case EASE_TYPE_SINE_OUT: {
		return ease_sine_out(t);
	} break;
	case EASE_TYPE_SINE_IN_OUT: {
		return ease_sine_in_out(t);
	} break;
	case EASE_TYPE_QUAD_IN: {
		return ease_quad_in(t);
	} break;
	case EASE_TYPE_QUAD_OUT: {
		return ease_quad_out(t);
	} break;
	case EASE_TYPE_QUAD_IN_OUT: {
		return ease_quad_in_out(t);
	} break;
	case EASE_TYPE_CUBIC_IN: {
		return ease_cubic_in(t);
	} break;
	case EASE_TYPE_CUBIC_OUT: {
		return ease_cubic_out(t);
	} break;
	case EASE_TYPE_CUBIC_IN_OUT: {
		return ease_cubic_in_out(t);
	} break;
	case EASE_TYPE_QUART_IN: {
		return ease_quart_in(t);
	} break;
	case EASE_TYPE_QUART_OUT: {
		return ease_quart_out(t);
	} break;
	case EASE_TYPE_QUART_IN_OUT: {
		return ease_quart_in_out(t);
	} break;
	case EASE_TYPE_QUINT_IN: {
		return ease_quint_in(t);
	} break;
	case EASE_TYPE_QUINT_OUT: {
		return ease_quint_out(t);
	} break;
	case EASE_TYPE_QUINT_IN_OUT: {
		return ease_quint_in_out(t);
	} break;
	case EASE_TYPE_EXPO_IN: {
		return ease_expo_in(t);
	} break;
	case EASE_TYPE_EXPO_OUT: {
		return ease_expo_out(t);
	} break;
	case EASE_TYPE_EXPO_IN_OUT: {
		return ease_expo_in_out(t);
	} break;
	case EASE_TYPE_CIRC_IN: {
		return ease_circ_in(t);
	} break;
	case EASE_TYPE_CIRC_OUT: {
		return ease_circ_out(t);
	} break;
	case EASE_TYPE_CIRC_IN_OUT: {
		return ease_circ_in_out(t);
	} break;
	case EASE_TYPE_BACK_IN: {
		return ease_back_in(t);
	} break;
	case EASE_TYPE_BACK_OUT: {
		return ease_back_out(t);
	} break;
	case EASE_TYPE_BACK_IN_OUT: {
		return ease_back_in_out(t);
	} break;
	case EASE_TYPE_ELASTIC_IN: {
		return ease_elastic_in(t);
	} break;
	case EASE_TYPE_ELASTIC_OUT: {
		return ease_elastic_out(t);
	} break;
	case EASE_TYPE_ELASTIC_IN_OUT: {
		return ease_elastic_in_out(t);
	} break;
	case EASE_TYPE_BOUNCE_IN: {
		return ease_bounce_in(t);
	} break;
	case EASE_TYPE_BOUNCE_OUT: {
		return ease_bounce_out(t);
	} break;
	case EASE_TYPE_BOUNCE_IN_OUT: {
		return ease_bounce_in_out(t);
	} break;
	default: {
		BAD_PATH;
	} break;
	}
	return t;
}
