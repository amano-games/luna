#pragma once

#include "base/mem.h"
#include "base/types.h"
#include "engine/gfx/gfx-defs.h"

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
void tex_opaque_to_rgba(struct tex tex, u32 *out, size size, struct gfx_col_pallete pallete);
void tex_cpy(struct tex *dst, struct tex *src);
void tex_opaque_to_pdi(struct tex tex, u8 *px, i32 bw, i32 bh, i32 bb);
