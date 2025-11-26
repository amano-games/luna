#include "aseprite.h"
#include "base/arr.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/str.h"
#include "engine/animation/animation-db.h"
#include "engine/animation/animation.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "tools/tsj/tsj.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "external/cute_aseprite.h"

static inline str8 str8_skip_non_alpha(str8 str);
static inline str8 str8_skip_until_assets(str8 str);

b32
aseprite_to_assets(const str8 in_path, const str8 out_path, struct alloc scratch)
{
	b32 res    = false;
	ase_t *ase = cute_aseprite_load_from_file((char *)in_path.str, NULL);
	dbg_check(ase, "aseprite", "failed to load file %s", in_path.str);

	aseprite_to_tex(ase, in_path, out_path, scratch);
	aseprite_to_ani(ase, in_path, out_path, scratch);

	res = true;

error:;
	if(ase) {
		cute_aseprite_free(ase);
	}
	return res;
}

b32
aseprite_to_tex(const ase_t *ase, const str8 in_path, const str8 out_path, struct alloc scratch)
{
	b32 res                     = false;
	struct pixel_u8 *sheet_data = NULL;
	str8 out_file_path          = path_make_file_name_with_ext(scratch, out_path, str8_lit(TEX_EXT));
	i32 sheet_w                 = ase->w * ase->frame_count;
	i32 sheet_h                 = ase->h;
	sheet_data                  = sys_alloc(NULL, sheet_w * sheet_h * sizeof(struct pixel_u8));
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

	res = tex_from_rgba_w((const struct pixel_u8 *)sheet_data, sheet_w, sheet_h, out_file_path);
	dbg_check(res, "ase", "failed to write tex file %s", out_file_path.str);
	log_info("ase-tex", "%s -> %s", in_path.str, out_path.str);

error:;
	if(sheet_data != NULL) {
		sys_free(sheet_data);
	}
	return res;
}

b32
aseprite_to_ani(
	const ase_t *ase,
	const str8 in_path,
	const str8 out_path,
	struct alloc scratch)
{
	b32 res    = false;
	void *file = NULL;

	str8 out_file_path = path_make_file_name_with_ext(scratch, out_path, str8_lit(ANIMATION_DB_EXT));

	struct ani_db db = {.bank_count = 1, .clip_count = ase->tag_count};
	db.assets        = arr_new(db.assets, 1, scratch);
	// in_path  = ./src/assets/img/frog.aseprite
	// out_path = ./tmp
	// res      = assets/img/frog.ani_db
	str8 asset_path           = path_make_file_name_with_ext(scratch, str8_skip_until_assets(in_path), str8_lit(TEX_EXT));
	struct ani_db_asset asset = {
		.path = asset_path,
		.info = {
			.cell_size = {ase->w, ase->h},
			.tex_size  = {ase->w * ase->frame_count, ase->h},
			.path_hash = hash_string(asset_path),
		},
	};
	asset.clips = arr_new(asset.clips, ase->tag_count, scratch);
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
		arr_push(asset.clips, clip);
	}
	arr_push(db.assets, asset);

	file = sys_file_open_w(out_file_path);
	dbg_check(file, "ani db", "failed to open file to write: %s", out_file_path.str);
	struct ser_writer w = {.f = file};
	ani_db_write(&w, db);
	log_info("ase-ani", "%s -> %s", in_path.str, out_path.str);

error:;
	if(file != NULL) {
		sys_file_close(file);
	}
	return res;
}

static inline str8
str8_skip_non_alpha(str8 str)
{
	str8 res  = str;
	u8 *first = str.str;
	u8 *opl   = first + str.size;
	for(; first < opl; first += 1) {
		if(char_is_alpha(*first)) {
			break;
		}
	}
	res = str8_range(first, opl);
	return (res);
}

static inline str8
str8_skip_until_assets(str8 str)
{
	str8 res  = str;
	ssize idx = str8_find_needle(str, 0, str8_lit("assets"), 0);
	if(idx > 0) {
		res = str8_skip(str, idx);
	}
	return (res);
}
