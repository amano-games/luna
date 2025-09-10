#pragma once

#include "base/types.h"
#include "mathfunc.h"

f32
halflife_to_damping(f32 halflife, f32 eps)
{
	if(eps == 0) {
		eps = 1e-5f;
	}
	return (4.0f * 0.69314718056f) / (halflife + eps);
}

f32
damping_to_halflife(f32 damping, f32 eps)
{
	if(eps == 0) {
		eps = 1e-5f;
	}
	return (4.0f * 0.69314718056f) / (damping + eps);
}

f32
frequency_to_stiffness(f32 frequency)
{
	return 0.0f;
	// return squaref(2.0f * PI_FLOAT * frequency);
}

f32
stiffness_to_frequency(f32 stiffness)
{
	return sqrt_f32(stiffness) / (2.0f * PI_FLOAT);
}
