#pragma once

#include "base/mathfunc.h"
#include "base/types.h"

// Wraps 49.7 days
// https://tomforsyth1000.github.io/blog.wiki.html#%5B%5BA%20matter%20of%20precision%5D%5D
struct tick_ms {
	u32 ms;
};

#define TICK_MS_ZERO ((struct tick_ms){0})

static inline i32
tick_ms_round_sec(f32 sec)
{
	return (i32)(sec * 1000.0f + (sec >= 0.0f ? 0.5f : -0.5f));
}

// t + seconds
static inline struct tick_ms
tick_ms_add_sec(struct tick_ms t, f32 sec)
{
	return (struct tick_ms){t.ms + (u32)tick_ms_round_sec(sec)};
}

// a - b in seconds, negative when a is before b
static inline f32
tick_ms_sec_between(struct tick_ms a, struct tick_ms b)
{
	return (f32)(i32)(a.ms - b.ms) * 1e-3f;
}

// a < b
static inline b32
tick_ms_lt(struct tick_ms a, struct tick_ms b)
{
	return (i32)(a.ms - b.ms) < 0;
}

// a > b
static inline b32
tick_ms_gt(struct tick_ms a, struct tick_ms b)
{
	return (i32)(a.ms - b.ms) > 0;
}

// a <= b
static inline b32
tick_ms_le(struct tick_ms a, struct tick_ms b)
{
	return (i32)(a.ms - b.ms) <= 0;
}

// a >= b
static inline b32
tick_ms_ge(struct tick_ms a, struct tick_ms b)
{
	return (i32)(a.ms - b.ms) >= 0;
}

static inline b32
tick_ms_eq(struct tick_ms a, struct tick_ms b)
{
	return a.ms == b.ms;
}

// 0 is used as "not set" by callers
static inline b32
tick_ms_is_zero(struct tick_ms t)
{
	return t.ms == 0;
}

// Angle in [0, 2pi)
static inline f32
tick_ms_phase(struct tick_ms t, f32 angular_freq)
{
	const f32 tau = PI2_FLOAT;
	if(angular_freq <= 0.0f) { return 0.0f; }
	u32 period_ms = (u32)(tau * 1000.0f / angular_freq + 0.5f);
	if(period_ms == 0) { return 0.0f; }
	return (f32)(t.ms % period_ms) * (tau / (f32)period_ms);
}

static inline f32
tick_ms_to_sec_dbg(struct tick_ms t)
{
	return (f32)t.ms * 1e-3f;
}

static inline f32
tick_ms_us_to_sec(u64 us)
{
	return (f32)us * 1e-6f;
}

static inline struct tick_ms
tick_ms_from_us(u64 us)
{
	return (struct tick_ms){(u32)(us / 1000u)};
}
