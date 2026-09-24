#pragma once

#include "base/types.h"
#include "lib/tex/tex.h"

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
