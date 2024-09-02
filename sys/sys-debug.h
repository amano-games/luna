#pragma once

#include "sys-types.h"

enum debug_shape_type {
	DEBUG_CIR,
	DEBUG_REC,
	DEBUG_POLY,
	DEBUG_LIN
};

struct debug_shape_cir {
	v2_i32 p;
	i32 r;
};

struct debug_shape_lin {
	v2_i32 a;
	v2_i32 b;
};

struct debug_shape_rec {
	i32 x, y, w, h;
};

struct debug_shape_poly {
	int count;
	v2_i32 verts[8];
};

struct debug_shape {
	enum debug_shape_type type;
	union {
		struct debug_shape_cir cir;
		struct debug_shape_lin lin;
		struct debug_shape_rec rec;
		struct debug_shape_poly poly;
	};
};

void sys_debug_draw(struct debug_shape *shapes, int count);
