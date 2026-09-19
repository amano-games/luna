#pragma once

#include "base/mem.h"
#include "base/types.h"
#include "engine/gfx/gfx-defs.h"

#define TEX_EXT "tex"

enum {
	TEX_FMT_OPAQUE, // only color pixels
	TEX_FMT_MASK,   // color and mask interlaced in words
};

enum {
	TEX_FLAG_NONE = 0,
	TEX_FLAG_LZ4  = 1 << 0,
};

struct pixel_u8 {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

struct tex_header {
	u32 fmt;
	u32 w;
	u32 h;
	u32 flags;
};

struct tex {
	u32 *px; // either black/white words or black/white and transparent/opaque words interlaced
	int wword;
	int fmt;
	int w;
	int h;
};

struct tex tex_create(struct alloc alloc, i32 w, i32 h);
struct tex tex_create_opaque(struct alloc alloc, i32 w, i32 h);
struct tex tex_load(struct alloc alloc, struct alloc scratch, str8 path);
struct tex tex_load_from_mem(struct alloc alloc, void *data, ssize size);
void tex_clr(struct tex dst, i32 col);
i32 tex_px_at(struct tex tex, i32 x, i32 y);
i32 tex_mask_at(struct tex tex, i32 x, i32 y);
void tex_px(struct tex tex, i32 x, i32 y, i32 col);
void tex_mask(struct tex tex, i32 x, i32 y, i32 col);

void tex_opaque_to_rgba(struct tex tex, u32 *out, ssize size, struct gfx_col_pallete pallete);
void tex_opaque_to_pdi(struct tex tex, u8 *px_out, i32 bw, i32 bh, i32 bb);
void tex_mask_to_pdi(struct tex tex, u8 *px_out, u8 *mask_out, i32 w, i32 h, i32 row_bytes);

void tex_cpy(struct tex *dst, struct tex *src);
struct tex tex_from_rgb(struct alloc alloc, const struct pixel_u8 *in_data, i32 w, i32 h);
