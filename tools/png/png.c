#include "png.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/log.h"
#include "base/str.h"
#include "lib/tex/tex.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO: detect transparency and save if the texture is opaque or not
b32
png_to_tex(
	const str8 in_path,
	const str8 out_path,
	struct alloc scratch)
{
	b32 res = false;
	i32 w, h, n;
	u32 *data = (u32 *)stbi_load((char *)in_path.str, &w, &h, &n, 4);

	dbg_check(data != NULL, "png", "Failed to load image with path %s: %s", in_path.str, stbi_failure_reason());

	str8 out_file_path = path_make_file_name_with_ext(scratch, out_path, str8_lit(TEX_EXT));

	res = tex_from_rgba_w((const struct pixel_u8 *)data, w, h, out_file_path);
	dbg_check(res, "png", "failed to write tex file %s", out_file_path.str);
	log_info("png", "%s -> %s", in_path.str, out_file_path.str);

error:;
	if(data != NULL) { stbi_image_free(data); }
	return res;
}
