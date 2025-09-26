#pragma once

#include "base/types.h"
enum gfx_col {
	GFX_COL_BLACK,
	GFX_COL_WHITE,
	GFX_COL_CLEAR,

	GFX_COL_NUM_COUNT,
};

struct gfx_col_pallete {
	u32 colors[GFX_COL_NUM_COUNT];
};
