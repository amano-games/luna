#include "png.h"
#include "base/dbg.h"
#include "base/ht.h"
#include "base/mem.h"
#include "lib/tex/tex.h"
#include "tools/asset/asset.h"
#include "tools/asset/asset-defs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

u8
find_color_index(struct gfx_col_palette_map palette_map, u32 color)
{
	u8 res = 0;
	// TODO: closest color to palette
	for(ssize i = 0; i < palette_map.palette.len; ++i) {
		if(color == palette_map.palette.colors[i]) {
			res = i;
			break;
		}
	}
	return res;
}

struct tex
tex_u8_from_rgba(
	struct alloc alloc,
	const struct pixel_u8 *in_data,
	i32 w,
	i32 h,
	struct gfx_col_palette_map palette_map)
{
	struct tex t = tex_create(alloc, w, h, TEX_FMT_8B_INDEX);
	if(!t.pxu8) { return t; }
	ssize stride = (ssize)t.wword * sizeof(u32);
	mset(t.pxu8, 0, stride * h);
	for(ssize y = 0; y < h; ++y) {
		const struct pixel_u8 *row = in_data + y * w;
		for(i32 x = 0; x < w; ++x) {
			struct pixel_u8 pixel = row[x];
			u32 color             = ((u32)pixel.r << 24) | ((u32)pixel.g << 16) | ((u32)pixel.b << 8) | (u32)pixel.a;
			// Reserve zero for empty keys and cache misses, including transparent black.
			u64 key   = (u64)color + 1;
			u32 value = ht_get_u32(&palette_map.ht, key);
			if(value == 0) {
				u8 index = find_color_index(palette_map, color);
				value    = (u32)index + 1;
				if(index < palette_map.palette.len) {
					ht_set_u32(&palette_map.ht, key, value);
				}
			}
			t.pxu8[y * stride + x] = (u8)(value - 1);
		}
	}
	return t;
}

b32
png_to_tex_blob(
	str8 in_path,
	struct alloc scratch,
	struct alloc alloc,
	struct asset_blob *out,
	struct gfx_col_palette_map palette_map,
	enum asset_flag flag)
{
	b32 res = false;
	i32 w, h, n;
	u32 *data = (u32 *)stbi_load((char *)in_path.str, &w, &h, &n, 4);
	dbg_check(data != NULL, "png", "Failed to load image with path %s: %s", in_path.str, stbi_failure_reason());

	const struct pixel_u8 *in_data = (const struct pixel_u8 *)data;
	struct tex t                   = {0};
	if(palette_map.palette.len > 2) {
		t = tex_u8_from_rgba(scratch, in_data, w, h, palette_map);
	} else {
		t = tex_1b_from_rgb(scratch, in_data, w, h);
	}
	res = tex_to_blob(scratch, alloc, t, out, flag);

error:;
	if(data != NULL) { stbi_image_free(data); }
	return res;
}
