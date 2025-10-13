#include "png.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/str.h"
#include "lib/tex/tex.h"
#include "sys/sys-intrin.h"
#include "sys/sys-io.h"
#include "base/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void
tex_handle(
	const str8 in_path,
	const str8 out_path,
	struct alloc scratch)
{
	i32 w, h, n;
	u32 *data = (u32 *)stbi_load((char *)in_path.str, &w, &h, &n, 4);

	dbg_check(data != NULL, "tex", "Failed to load image with path %s: %s", in_path.str, stbi_failure_reason());

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(TEX_EXT));
	i32 width_aligned  = (w + 31) & ~31;
	u32 color_row      = 0;
	u32 mask_row       = 0;
	size bit_idx       = 0;

	void *file = sys_file_open_w(out_file_path);

	dbg_check(file, "tex-gen", "failed to open file %s", out_file_path.str);

	// TODO: Detect if the texture has transparency
	struct tex_header header = {.w = w, .h = h};

	dbg_check(sys_file_w(file, &header, sizeof(header)) == 1, "tex-gen", "Error writing header image to file");

	for(i32 y = 0; y < h; ++y) {
		u32 *row = data + y * w;
		for(i32 x = 0; x < width_aligned; ++x) {
			struct tex_pixel pixel = {0};

			if(x < w) {
				pixel = *(struct tex_pixel *)(row + x);
			}

			i32 color      = (pixel.r + pixel.g + pixel.b) > (3 * 127);
			i32 alpha      = pixel.a > 127;
			u32 pixel_mask = (1u << (31 - bit_idx));

			if(color) { color_row |= pixel_mask; }
			if(alpha) { mask_row |= pixel_mask; }

			if(++bit_idx == 32) {
				color_row = bswap_u32(color_row);
				mask_row  = bswap_u32(mask_row);

				dbg_check(sys_file_w(file, &color_row, sizeof(u32)), "tex-gen", "failed to write color data to file");
				dbg_check(sys_file_w(file, &mask_row, sizeof(u32)), "tex-gen", "failed to write mask data to file");

				color_row = 0;
				mask_row  = 0;
				bit_idx   = 0;
			}
		}
		// flush remainder bits if width not multiple of 32
		if(bit_idx > 0) {
			color_row = bswap_u32(color_row);
			mask_row  = bswap_u32(mask_row);
			dbg_check(sys_file_w(file, &color_row, sizeof(u32)) == 1,
				"tex-gen",
				"failed to write color data (partial)");
			dbg_check(sys_file_w(file, &mask_row, sizeof(u32)) == 1,
				"tex-gen",
				"failed to write mask data (partial)");
			color_row = mask_row = 0;
			bit_idx              = 0;
		}
	}

	log_info("tex-gen", "%s -> %s\n", in_path.str, out_file_path.str);

error:;
	if(file) { sys_file_close(file); }
	if(data != NULL) { stbi_image_free(data); }
	return;
}
