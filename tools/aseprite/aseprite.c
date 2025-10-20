#include "aseprite.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-intrin.h"
#include "sys/sys-io.h"
#include "sys/sys.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "external/cute_aseprite.h"

b32
aseprite_to_tex(const str8 in_path, const str8 out_path, struct alloc scratch)
{
	b32 res    = false;
	void *file = NULL;
	ase_t *ase = cute_aseprite_load_from_file((char *)in_path.str, NULL);
	void *data = NULL;
	dbg_check(ase, "aseprite", "failed to load file %s", in_path.str);

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(TEX_EXT));
	i32 cell_h         = ase->h;
	i32 cell_w         = ase->w;
	i32 cell_w_aligned = (cell_w + 31) & ~31;
	i32 w_total        = cell_w * ase->frame_count;
	i32 w_word         = (w_total / 32) << 1; // TODO Opaque
	size mem_size      = sizeof(u32) * w_word * cell_h * 2;
	size bit_idx       = 0;
	u32 color_row      = 0;
	u32 mask_row       = 0;
	data               = sys_alloc(NULL, w_word);
	dbg_check(data, "aseprite", "failed to get memory for file %s", in_path.str);

	file = sys_file_open_w(out_file_path);
	dbg_check(file, "tex-gen", "failed to open file %s", out_file_path.str);

	struct tex_header header = {.w = w_total, .h = cell_h};
	dbg_check(sys_file_w(file, &header, sizeof(header)) == 1, "tex-gen", "Error writing header image to file");
	for(size y = 0; y < ase->h; ++y) {
		for(int i = 0; i < ase->frame_count; ++i) {
			ase_frame_t *frame  = ase->frames + i;
			ase_color_t *pixels = frame->pixels;
			ase_color_t *row    = pixels + y * cell_w;
			for(i32 x = 0; x < cell_w_aligned; ++x) {
				struct ase_color_t pixel = {0};
				if(x < cell_w) {
					pixel = row[x];
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
	}
	res = true;
	log_info("ase", "%s -> %s", in_path.str, out_file_path.str);

error:;
	if(ase != NULL) {
		cute_aseprite_free(ase);
	}
	if(data != NULL) {
		sys_free(data);
	}
	if(file != NULL) {
		sys_file_close(file);
	}
	return res;
}
