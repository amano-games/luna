#pragma once

#include "base/types.h"
#include "lib/tex/tex.h"

#define FNT_ASCII_MAX      128
#define FNT_ASCII_KERN_MIN 32 // Skip non visual chars
#define FNT_ASCII_KERN_MAX (FNT_ASCII_MAX - FNT_ASCII_KERN_MIN)

struct fnt_metrics {
	i8 baseline;
	i8 x_height;
	i8 cap_height;
	i8 descent;
};

struct fnt_ascii {
	u64 mask[FNT_ASCII_MAX / 64];
	u16 cells[FNT_ASCII_MAX];
	u8 widths[FNT_ASCII_MAX];
	i8 *kern; // FNT_ASCII_KERN_MAX * FNT_ASCII_KERN_MAX
};

struct fnt_utf8 {
	u32 count;
	u32 *code_points;
	u16 *cells;
	u8 *widths;

	// Kerning pairs sorted by key
	u32 kern_count;
	u64 *kern_keys; // ((u64)a << 32) | b
	i8 *kern_vals;
};

struct fnt {
	struct tex t;
	i8 tracking;
	u16 grid_w; // Num of columns
	u16 grid_h; // Num of rows
	u16 cell_w; // Width of cell
	u16 cell_h; // Height of cell
	struct fnt_metrics metrics;

	struct fnt_ascii *ascii;
	struct fnt_utf8 *utf8;
};
