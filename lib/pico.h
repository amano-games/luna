#pragma once

#include "mathfunc.h"
#include "sys-types.h"

static inline f32
pico_sin_f32(f32 value)
{
	f32 res = -sin_f32(value * PI2_FLOAT);
	return res;
}
