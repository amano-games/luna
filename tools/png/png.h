#pragma once

#include "base/mem.h"
#include "base/types.h"

struct tex_pixel {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

#define TEX_EXT "tex"

void tex_handle(const str8 in_path, const str8 out_path, struct alloc scratch);
