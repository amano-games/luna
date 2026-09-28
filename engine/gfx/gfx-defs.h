#pragma once

#include "base/ht.h"
#include "base/types.h"

enum gfx_col {
	GFX_COL_BLACK,
	GFX_COL_WHITE,
	GFX_COL_CLEAR,

	GFX_COL_NUM_COUNT,
};

struct gfx_col_palette {
	ssize len;
	u32 colors[256];
};

struct gfx_col_palette_map {
	struct ht_u32 ht;
	struct gfx_col_palette palette;
};
