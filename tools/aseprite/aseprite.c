#include "aseprite.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "external/cute_aseprite.h"

b32
aseprite_to_tex(const str8 in_path, const str8 out_path, struct alloc scratch)
{
	b32 res                     = false;
	void *file                  = NULL;
	ase_t *ase                  = cute_aseprite_load_from_file((char *)in_path.str, NULL);
	struct pixel_u8 *sheet_data = NULL;
	dbg_check(ase, "aseprite", "failed to load file %s", in_path.str);

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(TEX_EXT));

	i32 sheet_w = ase->w * ase->frame_count;
	i32 sheet_h = ase->h;
	sheet_data  = sys_alloc(NULL, sheet_w * sheet_h * sizeof(struct pixel_u8));
	{
		// Build horizontal sprite sheet
		{
			for(int i = 0; i < ase->frame_count; ++i) {
				ase_frame_t *frame  = ase->frames + i;
				ase_color_t *pixels = frame->pixels;
				for(size y = 0; y < ase->h; ++y) {
					for(size x = 0; x < ase->w; ++x) {
						size dst_x                      = x + i * ase->w;
						struct ase_color_t src          = pixels[y * ase->w + x];
						sheet_data[y * sheet_w + dst_x] = (struct pixel_u8){
							.r = src.r,
							.g = src.g,
							.b = src.b,
							.a = src.a,
						};
					}
				}
			}
		}
	}

	// TODO: generate animations

	res = tex_from_rgba_w((const struct pixel_u8 *)sheet_data, sheet_w, sheet_h, out_file_path);
	dbg_check(res, "ase", "failed to write tex file %s", out_file_path.str);
	log_info("ase", "%s -> %s", in_path.str, out_file_path.str);

error:;
	if(ase != NULL) {
		cute_aseprite_free(ase);
	}
	if(sheet_data != NULL) {
		sys_free(sheet_data);
	}
	if(file != NULL) {
		sys_file_close(file);
	}
	return res;
}
