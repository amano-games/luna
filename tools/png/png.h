#pragma once

#include "base/mem.h"
#include "base/types.h"

struct tex_pixel {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

b32 png_to_tex(const str8 in_path, const str8 out_path, struct alloc scratch);
