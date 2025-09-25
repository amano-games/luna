#pragma once

#include "base/types.h"
#include "base/v2.h"

static inline v2
quad_bezier_v2(v2 a, v2 b, v2 c, f32 t)
{
	v2 ab = v2_lerp(a, b, t);
	v2 bc = v2_lerp(b, c, t);
	return v2_lerp(ab, bc, t);
}
