#pragma once

#include "base/ht.h"
#include "base/types.h"

// Values are 8-bit palette indices: asset-gen reserves 0 for clear
enum gfx_col {
	GFX_COL_CLEAR,
	GFX_COL_BLACK,
	GFX_COL_WHITE,

	GFX_COL_NUM_COUNT,
};

struct gfx_col_palette {
	ssize len;
	u32 colors[256];
};

struct gfx_col_lut {
	// colors the tex actually has, for example in a grayscale input
	// color 0 is black, color 1 is gray, color 2 is white
	struct gfx_col_palette in;
	// cache to avoid searching all the colors of the image again
	// So for example if a tex has a slight red and the palette has pure red, we need to go through all the colors in the palette_in and find the closest color, we cache this in the ht_in
	struct ht_u32 ht;
	// Colors that the tex should have at the end of the convertion
	// so for a playdate palette it's black and white.
	// We calculate this once searching all unique colors on the second row of the palette map texture.
	// They can be repeated as 2 colors in palette_in can map to 1 single color in palette_out
	struct gfx_col_palette out;
	// The actual map, color from palette_in
	// palette_out = 0 1
	// palette_in  = 1 2 3 4 5
	// palette_map = 0 1 1 0 1
	struct gfx_col_palette map;
};
