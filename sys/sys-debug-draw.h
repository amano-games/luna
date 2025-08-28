#pragma once

#include "sys-types.h"

enum debug_shape_type {
	DEBUG_CIR,
	DEBUG_REC,
	DEBUG_POLY,
	DEBUG_LIN,
	DEBUG_ELLIPSIS,
};

struct debug_shape_cir {
	b32 filled;
	v2_i32 p;
	i32 d;
};

struct debug_shape_lin {
	v2_i32 a;
	v2_i32 b;
};

struct debug_shape_rec {
	b32 filled;
	i32 x, y, w, h;
};

struct debug_shape_poly {
	int count;
	v2_i32 verts[8];
};

struct debug_shape_ellipsis {
	i32 x, y;
	i32 rx, ry;
};

struct debug_shape {
	enum debug_shape_type type;
	union {
		struct debug_shape_cir cir;
		struct debug_shape_lin lin;
		struct debug_shape_rec rec;
		struct debug_shape_poly poly;
		struct debug_shape_ellipsis ellipsis;
	};
};

void sys_debug_draw(struct debug_shape *shapes, int count);
