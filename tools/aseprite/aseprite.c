#include "aseprite.h"
#include "base/arr.h"
#include "base/dbg.h"
#include "base/log.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "engine/animation/animation-clips.h"
#include "engine/animation/animation.h"
#include "engine/assets/tex-atlas.h"
#include "lib/tex/tex.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "tools/asset/asset-defs.h"
#include "tools/asset/asset.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "external/cute_aseprite.h"

static void
aseprite_to_atlas(const ase_t *ase, const str8 out_path, struct alloc scratch)
{
	v2_i32 cell_size    = {ase->w, ase->h};
	str8 atlas_path     = path_make_file_name_with_ext(scratch, out_path, str8_lit(ATLAS_EXT));
	sys_file file       = sys_file_zero();
	struct ser_writer w = {0};

	file = sys_file_open_w(atlas_path);
	dbg_check(sys_file_is_valid(file), "tex-atlas", "can't write %s", atlas_path.str);
	w.f = file;
	atlas_write(&w, (struct tex_atlas){
		.cell_size = cell_size,
	});
	log_info("tex-atlas", "%s cell=%dx%d", atlas_path.str, cell_size.x, cell_size.y);

error:;
	if(sys_file_is_valid(file)) {
		sys_file_close(file);
	}
}

b32
aseprite_to_assets(const str8 in_path, const str8 out_path, struct alloc scratch, enum tex_px_enc enc)
{
	b32 res    = false;
	ase_t *ase = cute_aseprite_load_from_file((char *)in_path.str, NULL);
	dbg_check(ase, "aseprite", "failed to load file %s", in_path.str);
	struct asset_blob blob = {0};

	{
		str8 out_file_path = path_make_file_name_with_ext(scratch, out_path, str8_lit(TEX_EXT));
		aseprite_to_tex(ase, scratch, sys_allocator(), &blob, enc);
		res = asset_blob_w(blob, out_file_path);
	}
	aseprite_to_atlas(ase, out_path, scratch);
	aseprite_to_ani(ase, out_path, scratch);

	res = true;

error:;
	if(blob.data) {
		sys_free(blob.data);
	}
	if(ase) {
		cute_aseprite_free(ase);
	}
	return res;
}

b32
aseprite_to_tex(const ase_t *ase, struct alloc scratch, struct alloc alloc, struct asset_blob *out, enum tex_px_enc enc)
{
	b32 res                     = false;
	i32 sheet_w                 = ase->w * ase->frame_count;
	i32 sheet_h                 = ase->h;
	struct pixel_u8 *sheet_data = alloc_arr(alloc, sheet_data, sheet_w * sheet_h);
	dbg_check_mem(sheet_data, "ase");
	{
		// Build horizontal sprite sheet
		{
			for(int i = 0; i < ase->frame_count; ++i) {
				ase_frame_t *frame  = ase->frames + i;
				ase_color_t *pixels = frame->pixels;
				for(ssize y = 0; y < ase->h; ++y) {
					for(ssize x = 0; x < ase->w; ++x) {
						ssize dst_x                     = x + i * ase->w;
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

	const struct pixel_u8 *in_data = (const struct pixel_u8 *)sheet_data;
	struct tex t                   = tex_from_rgb(scratch, in_data, sheet_w, sheet_h);
	res                            = tex_to_blob(scratch, alloc, t, out, enc);

error:;
	if(sheet_data != NULL) {
		sys_free(sheet_data);
	}
	return res;
}

b32
aseprite_to_ani(const ase_t *ase, const str8 out_path, struct alloc scratch)
{
	b32 res                      = false;
	sys_file file                = sys_file_zero();
	struct animation_clip *clips = NULL;
	str8 out_file_path          = {0};
	struct ser_writer w         = {0};

	if(ase->tag_count > 0) {
		clips = arr_new(scratch, clips, ase->tag_count);
		for(ssize i = 0; i < ase->tag_count; ++i) {
			const ase_tag_t *tag       = ase->tags + i;
			struct animation_clip clip = {0};
			i32 frame_count            = tag->to_frame - tag->from_frame + 1;
			clip.count                 = tag->repeat;
			clip.frame_duration        = ase->frames[tag->from_frame].duration_milliseconds / 1000.0f;
			clip.scale                 = 1.0f;
			{
				struct animation_track *track = &clip.tracks[ANIMATION_TRACK_FRAME - 1];
				track->type                   = ANIMATION_TRACK_FRAME;
				track->frames.len             = frame_count;
				track->frames.cap             = frame_count;
				for(ssize j = 0; j < frame_count; ++j) {
					track->frames.items[j] = tag->from_frame + j;
				}
			}
			{
				struct animation_track *track = &clip.tracks[ANIMATION_TRACK_SPRITE_MODE - 1];
				track->type                   = ANIMATION_TRACK_SPRITE_MODE;
				track->frames.len             = 1;
				track->frames.cap             = 1;
			}
			dbg_assert(clip.frame_duration > 0);
			dbg_assert(clip.frame_duration < 10);
			dbg_assert(clip.tracks[0].frames.len > 0 || clip.tracks[1].frames.len > 0);
			arr_push(clips, clip);
		}

		out_file_path = path_make_file_name_with_ext(scratch, out_path, str8_lit(ANI_EXT));
		file          = sys_file_open_w(out_file_path);
		dbg_check(sys_file_is_valid(file), "ase-ani", "failed to open file to write: %s", out_file_path.str);
		w.f = file;
		ani_clips_write(&w, clips);
		log_info("ase-ani", "%s", out_file_path.str);
	}

	res = true;

error:;
	if(sys_file_is_valid(file)) {
		sys_file_close(file);
	}
	return res;
}
