#pragma once

#include "gfx/gfx.h"
#include "serialize/serialize.h"
#include "sys-types.h"

#define FNT_CHAR_MAX       256
#define FNT_KERN_PAIRS_MAX U16_MAX

struct fnt_metrics {
	i8 baseline;
	i8 x_height;
	i8 cap_height;
	i8 descent;
};

struct fnt {
	struct tex t;
	i8 tracking;
	u16 grid_w; // Num of columns
	u16 grid_h; // Num of rows
	u16 cell_w; // Width of cell
	u16 cell_h; // Height of cell
	struct fnt_metrics metrics;
	u8 *widths;
	i8 *kern_pairs;
};

i32 fnt_char_size_x_px(struct fnt fnt, i32 a, i32 b, i32 tracking);
v2_i32 fnt_size_px(struct fnt fnt, const str8 str, i32 tracking, i32 leading);

void fnt_write(struct fnt fnt, struct ser_writer *w);
i32 fnt_read(struct ser_reader *r, struct fnt *fnt);
struct fnt fnt_load(str8 path, struct alloc alloc, struct alloc scratch);
