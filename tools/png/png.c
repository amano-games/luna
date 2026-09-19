#include "png.h"
#include "base/dbg.h"
#include "base/mem.h"
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
	enum tex_px_enc enc)
{
	b32 res = false;
	i32 w, h, n;
	u32 *data = (u32 *)stbi_load((char *)in_path.str, &w, &h, &n, 4);
	dbg_check(data != NULL, "png", "Failed to load image with path %s: %s", in_path.str, stbi_failure_reason());

	const struct pixel_u8 *in_data = (const struct pixel_u8 *)data;
	struct tex t                   = tex_from_rgb(scratch, in_data, w, h);
	res                            = tex_to_blob(scratch, alloc, t, out, enc);

error:;
	if(data != NULL) { stbi_image_free(data); }
	return res;
}
