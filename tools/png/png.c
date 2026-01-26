#include "png.h"
#include "base/dbg.h"
#include "base/mem.h"
#include "lib/tex/tex.h"
#include "tools/asset/asset-defs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

b32
png_to_tex_blob(
	str8 in_path,
	struct alloc scratch,
	struct alloc alloc,
	struct asset_blob *out)
{
	b32 res = false;
	i32 w, h, n;
	u32 *data = (u32 *)stbi_load((char *)in_path.str, &w, &h, &n, 4);
	dbg_check(data != NULL, "png", "Failed to load image with path %s: %s", in_path.str, stbi_failure_reason());

	const struct pixel_u8 *in_data = (const struct pixel_u8 *)data;

	ssize out_size = tex_from_rgb(in_data, w, h, NULL, 0);
	dbg_check(out_size > 0, "png", "Invalid tex size");

	void *out_data = alloc_size(alloc, out_size, 4, false);
	dbg_check_mem(out_data, "png");

	dbg_check(tex_from_rgb(in_data, w, h, out_data, out_size) == out_size, "png", "convertion failed");

	out->data = out_data;
	out->size = out_size;
	res       = true;

error:;
	if(data != NULL) { stbi_image_free(data); }
	return res;
}
