#pragma once

#include "mem.h"
#include "sys-types.h"

enum {
	TEX_FMT_OPAQUE, // only color pixels
	TEX_FMT_MASK,   // color and mask interlaced in words
};

struct tex {
	u32 *px; // either black/white words or black/white and transparent/opaque words interlaced
	int wword;
	int fmt;
	int w;
	int h;
};

struct tex tex_create(i32 w, i32 h, struct alloc alloc);
struct tex tex_create_opaque(i32 w, i32 h, struct alloc alloc);
struct tex tex_load(str8 path, struct alloc alloc);
void tex_clr(struct tex dst, i32 col);
i32 tex_px_at(struct tex tex, i32 x, i32 y);
i32 tex_mask_at(struct tex tex, i32 x, i32 y);
void tex_px(struct tex tex, i32 x, i32 y, i32 col);
void tex_mask(struct tex tex, i32 x, i32 y, i32 col);
