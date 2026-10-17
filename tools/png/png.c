#include "png.h"
#include "base/dbg.h"
#include "base/ht.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/v3.h"
#include "lib/color.h"
#include "lib/tex/tex.h"
#include "tools/asset/asset.h"
#include "tools/asset/asset-defs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

b32
png_to_tex_blob(
	str8 in_path,
	struct alloc scratch,
	struct alloc alloc,
	struct asset_blob *out,
	struct gfx_col_lut *col_map,
	enum asset_flag flag)
{
	b32 res = false;
	i32 w, h, n;
	u32 *data = (u32 *)stbi_load((char *)in_path.str, &w, &h, &n, 4);
	dbg_check(data != NULL, "png", "Failed to load image with path %.*s: %s", str8_spread(in_path), stbi_failure_reason());

	const struct pixel_u8 *in_data = (const struct pixel_u8 *)data;
	struct tex t                   = {0};
	if(col_map->ht.ht == NULL) {
		t = tex_1b_from_rgb(scratch, in_data, w, h);
	} else if(col_map->out.len > 3) {
		t = tex_u8_from_rgba(scratch, in_data, w, h, col_map);
	} else {
		t = tex_1b_from_rgb_lut(scratch, in_data, w, h, col_map);
	}
	dbg_check(t.pxu8 != NULL, "png", "Failed to convert image with path %.*s", str8_spread(in_path));
	res = tex_to_blob(scratch, alloc, t, out, flag);

error:;
	if(data != NULL) { stbi_image_free(data); }
	return res;
}
