#include "tsj.h"

#include "base/arr.h"
#include "base/dbg.h"
#include "lib/json.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "lib/serialize/serialize.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "base/log.h"
#include "base/types.h"
#include "base/utils.h"
#include "engine/animation/animation-clips.h"
#include "engine/assets/path-db.h"
#include "engine/assets/tex-atlas.h"
#include "sys/sys.h"
#include "tools/asset/asset.h"

static void tsj_atlas_gen(str8 src_root, str8 dest_root, str8 src_path, struct tex_atlas atlas, v2_i32 tex_size, struct alloc scratch);
static void tsj_ani_gen(str8 src_root, str8 dest_root, str8 src_path, struct animation_clip *clips, struct alloc scratch);

static str8
tsj_resolve_image(str8 rel, str8 tsj_path, struct alloc alloc, struct alloc scratch)
{
	str8 base_dir       = str8_chop_last_slash(tsj_path);
	str8 path_with_dots = str8_fmt_push(scratch, "%.*s/%.*s", str8_spread(base_dir), str8_spread(rel));
	return path_resolve_dots(alloc, path_with_dots, path_style_relative, scratch);
}

str8
tsj_handle_path(
	str8 path,
	str8 in_path,
	struct alloc alloc,
	struct alloc scratch)
{
	// in: ..\/demons\/demon-001.png
	// root: ./src/assets/map/catcha-diablos.tsj
	// out: assets/demons/demon-001.png
	str8 res    = tsj_resolve_image(path, in_path, scratch, scratch);
	res         = path_make_file_name_with_ext(alloc, res, str8_lit("tex"));
	str8 prefix = str8_lit("src/");
	res         = str8_skip(res, prefix.size);
	return res;
}

struct tsj_track_res
tsj_handle_track(str8 json,
	jsmntok_t *tokens,
	i32 index,
	struct alloc alloc,
	struct alloc scratch)
{
	struct tsj_track_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("frames")) == 0) {
			dbg_assert(value->type == JSMN_STRING);
			str8 frames_str = {
				.str  = json.str + value->start,
				.size = value->end - value->start,
			};
			struct str8_list list = str8_split(scratch, frames_str, (u8 *)",", 1, 0);
			i32 last_idx          = 0;
			for(struct str8_node *n = list.first; n != 0; n = n->next) {
				// NOTE: we remove 1 because in tiled we specify them starting with 1 instead of 0
				res.track.frames.items[last_idx++] = str8_to_i32(n->str) - 1;
			}

			res.track.frames.len = last_idx;
			res.track.frames.cap = last_idx;
		}
	}

	return res;
}

struct tsj_animation_res
tsj_handle_animation(str8 json,
	jsmntok_t *tokens,
	i32 index,
	struct alloc alloc,
	struct alloc scratch)
{
	struct tsj_animation_res res = {0};
	jsmntok_t *root              = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	str8 json_root = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	res.token_count = jsmn_parse(&parser, (const char *)json_root.str, json_root.size, NULL, 0);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];

		if(json_eq(json, key, str8_lit("count")) == 0) {
			res.clip.count = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("frame_duration")) == 0) {
			res.clip.frame_duration = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("scale")) == 0) {
			res.clip.scale = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("track_frame")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct tsj_track_res track_res                  = tsj_handle_track(json, tokens, i + 1, alloc, scratch);
			res.clip.tracks[ANIMATION_TRACK_FRAME - 1]      = track_res.track;
			res.clip.tracks[ANIMATION_TRACK_FRAME - 1].type = ANIMATION_TRACK_FRAME;
			i += track_res.token_count;
		} else if(json_eq(json, key, str8_lit("track_sprite_mode")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct tsj_track_res track_res                        = tsj_handle_track(json, tokens, i + 1, alloc, scratch);
			res.clip.tracks[ANIMATION_TRACK_SPRITE_MODE - 1]      = track_res.track;
			res.clip.tracks[ANIMATION_TRACK_SPRITE_MODE - 1].type = ANIMATION_TRACK_SPRITE_MODE;
			i += track_res.token_count;
		}
	}

	if(res.clip.count == 0) {
		res.clip.count = 1;
	}
	if(res.clip.scale == 0) {
		res.clip.scale = 1;
	}
	if(res.clip.frame_duration == 0) {
		res.clip.frame_duration = 100;
	}

	res.clip.frame_duration = res.clip.frame_duration / 1000;
	dbg_assert(res.clip.frame_duration > 0);
	dbg_assert(res.clip.frame_duration < 10);

	return res;
}

struct tsj_property_res
tsj_handle_property(
	str8 json,
	jsmntok_t *tokens,
	i32 index,
	struct alloc alloc,
	struct alloc scratch)
{
	struct tsj_property_res res = {0};
	jsmntok_t *root             = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	str8 json_root = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	res.token_count = jsmn_parse(&parser, (const char *)json_root.str, json_root.size, NULL, 0);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("propertytype")) == 0) {
			if(json_eq(json, value, str8_lit("animation")) != 0) {
				// property not supported
				return res;
			}
		} else if(json_eq(json, key, str8_lit("value")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			usize item_index                  = i + 1;
			struct tsj_animation_res item_res = tsj_handle_animation(
				json,
				tokens,
				item_index,
				alloc,
				scratch);
			res.clip = item_res.clip;
			dbg_assert(res.clip.count != 0);
			return res;
		}
	}

	return res;
}

struct tsj_tile_res
tsj_handle_tile(
	str8 in_path,
	str8 json,
	jsmntok_t *tokens,
	i32 index,
	struct alloc alloc,
	struct alloc scratch)
{
	struct tsj_tile_res res = {0};
	jsmntok_t *root         = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	str8 json_root = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	res.token_count = jsmn_parse(&parser, (const char *)json_root.str, json_root.size, NULL, 0);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("image")) == 0) {
			str8 path    = json_str8_cpy_push(json, value, scratch, 0);
			res.src_path = tsj_resolve_image(path, in_path, alloc, scratch);
			res.path     = tsj_handle_path(path, in_path, alloc, scratch);
		} else if(json_eq(json, key, str8_lit("width")) == 0) {
			res.atlas.cell_w = (u16)json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("height")) == 0) {
			res.atlas.cell_h = (u16)json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("imagewidth")) == 0) {
			res.tex_size.x = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("imageheight")) == 0) {
			res.tex_size.y = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("properties")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.clips = arr_new(alloc, res.clips, value->size);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index  = i + 2;
				jsmntok_t *item = &tokens[item_index];
				dbg_assert(item->type == JSMN_OBJECT);
				struct tsj_property_res item_res = tsj_handle_property(
					json,
					tokens,
					item_index,
					alloc,
					scratch);

				if(item_res.clip.scale > 0) {
					arr_push(res.clips, item_res.clip);
				}
				i += item_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("objectgroup")) == 0) {
			usize skip_count = json_obj_count(json, value);
			i += skip_count;
		} else {
		}
	}

	if(res.atlas.cell_w == 0) {
		res.atlas.cell_w = (u16)res.tex_size.x;
	}
	if(res.atlas.cell_h == 0) {
		res.atlas.cell_h = (u16)res.tex_size.y;
	}

	for(ssize j = 0; j < arr_len(res.clips); ++j) {
		struct animation_clip *clip = res.clips + j;
		if(clip->tracks[0].frames.len == 0 && res.atlas.cell_w) {
			ssize cells_count          = res.tex_size.x / res.atlas.cell_w;
			clip->tracks[0].frames.len = cells_count;
			for(ssize k = 0; k < cells_count; ++k) {
				clip->tracks[0].frames.items[k] = k;
			}
		}
	}

	return res;
}

str8 *
tsj_handle_json(
	str8 in_path,
	str8 json,
	str8 src_root,
	str8 dest_root,
	struct alloc alloc,
	struct alloc scratch)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(scratch, tokens, token_count);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_assert(json_res == token_count);

	jsmntok_t root = tokens[0];

	dbg_assert(root.type == JSMN_OBJECT);

	str8 *res = NULL;

	for(usize i = 0; i < (usize)token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("tilecount")) == 0) {
			usize count = json_parse_i32(json, value);
			res         = arr_new(alloc, res, count);
		} else if(json_eq(json, key, str8_lit("tiles")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			dbg_assert(arr_cap(res) == value->size);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index  = i + 2;
				jsmntok_t *item = &tokens[item_index];
				dbg_assert(item->type == JSMN_OBJECT);
				struct tsj_tile_res tile_res = tsj_handle_tile(
					in_path,
					json,
					tokens,
					item_index,
					alloc,
					scratch);
				if(tile_res.path.size > 0) {
					arr_push(res, tile_res.path);
				}
				if(tile_res.src_path.size > 0) {
					tsj_atlas_gen(src_root, dest_root, tile_res.src_path, tile_res.atlas, tile_res.tex_size, scratch);
					if(arr_len(tile_res.clips) > 0) {
						tsj_ani_gen(src_root, dest_root, tile_res.src_path, tile_res.clips, scratch);
					}
				}

				i += tile_res.token_count;
			}
		}
	}

	return res;
}

static void
tsj_make_parents(str8 file_path, struct alloc scratch)
{
	str8 dir = str8_cpy_push(scratch, str8_chop_last_slash(file_path));
	usize i;

	for(i = 1; i < dir.size; ++i) {
		if(dir.str[i] == '/' || dir.str[i] == '\\') {
			u8 saved   = dir.str[i];
			dir.str[i] = 0;
			sys_make_dir((str8){.str = dir.str, .size = i});
			dir.str[i] = saved;
		}
	}

	if(dir.size > 0) {
		sys_make_dir(dir);
	}
}

static void
tsj_atlas_gen(str8 src_root, str8 dest_root, str8 src_path, struct tex_atlas atlas, v2_i32 tex_size, struct alloc scratch)
{
	str8 src_n             = path_resolve_dots(scratch, src_path, path_style_relative, scratch);
	str8 root_n            = path_resolve_dots(scratch, src_root, path_style_relative, scratch);
	str8 rel               = {0};
	str8 out               = {0};
	struct asset_blob blob = {0};

	if(root_n.size > 0) {
		u8 last = root_n.str[root_n.size - 1];
		if(last == '/' || last == '\\') {
			root_n.size -= 1;
		}
	}

	dbg_assert(str8_starts_with(src_n, root_n, 0));
	rel = str8_skip(src_n, root_n.size);
	if(rel.size > 0 && (rel.str[0] == '/' || rel.str[0] == '\\')) {
		rel = str8_skip(rel, 1);
	}

	out = str8_fmt_push(scratch, "%.*s/%.*s", str8_spread(dest_root), str8_spread(rel));
	out = path_make_file_name_with_ext(scratch, out, str8_lit(ATLAS_EXT));

	if((i32)atlas.cell_w != tex_size.x || (i32)atlas.cell_h != tex_size.y) {
		tsj_make_parents(out, scratch);
		dbg_check(atlas_to_blob(scratch, atlas, &blob), "tex-atlas", "can't pack %s", out.str);
		dbg_check(asset_blob_w(blob, out), "tex-atlas", "can't write %s", out.str);
		log_info("tex-atlas", "%s cell=%dx%d", out.str, atlas.cell_w, atlas.cell_h);
	}

error:;
}

static void
tsj_ani_gen(str8 src_root, str8 dest_root, str8 src_path, struct animation_clip *clips, struct alloc scratch)
{
	str8 src_n          = path_resolve_dots(scratch, src_path, path_style_relative, scratch);
	str8 root_n         = path_resolve_dots(scratch, src_root, path_style_relative, scratch);
	str8 rel            = {0};
	str8 out            = {0};
	sys_file file       = sys_file_zero();
	struct ser_writer w = {0};

	if(root_n.size > 0) {
		u8 last = root_n.str[root_n.size - 1];
		if(last == '/' || last == '\\') {
			root_n.size -= 1;
		}
	}

	dbg_assert(str8_starts_with(src_n, root_n, 0));
	rel = str8_skip(src_n, root_n.size);
	if(rel.size > 0 && (rel.str[0] == '/' || rel.str[0] == '\\')) {
		rel = str8_skip(rel, 1);
	}

	out = str8_fmt_push(scratch, "%.*s/%.*s", str8_spread(dest_root), str8_spread(rel));
	out = path_make_file_name_with_ext(scratch, out, str8_lit(ANI_EXT));
	tsj_make_parents(out, scratch);

	file = sys_file_open_w(out);
	dbg_check(sys_file_is_valid(file), "ani", "can't write %s", out.str);
	w.f = file;
	ani_clips_write(&w, clips);
	log_info("ani", "%s clips=%d", out.str, (int)arr_len(clips));

error:;
	if(sys_file_is_valid(file)) {
		sys_file_close(file);
	}
}

i32
handle_tsj(str8 in_path, str8 out_path, str8 src_root, str8 dest_root, struct alloc scratch)
{
	i32 res                = 0;
	struct alloc alloc_sys = sys_allocator();
	usize mem_size         = MKILOBYTE(100);
	u8 *mem_buffer         = mem_alloc_size(alloc_sys, mem_size);
	dbg_assert(mem_buffer != NULL);
	struct marena marean = {0};
	marena_init(&marean, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marean);

	str8 out_file_path = path_make_file_name_with_ext(scratch, out_path, str8_lit(ADB_EXT));

	str8 json = {0};
	json_load(in_path, scratch, &json);
	str8 *paths = tsj_handle_json(in_path, json, src_root, dest_root, alloc, scratch);

	sys_file out_file = sys_file_open_w(out_file_path);
	if(!sys_file_is_valid(out_file)) {
		log_error("path-db-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	struct ser_writer w = {.f = out_file};
	path_db_write(&w, paths);

	sys_file_close(out_file);

#if 0
	{
		struct sys_full_file_res file_res = sys_load_full_file(out_file_path, scratch);
		struct ser_reader r               = {.data = file_res.data, .len = file_res.size};
		struct ser_value root             = ser_read(&r);
		str8 str                          = ser_value_to_str(scratch, &r, root);
		sys_printf("%s", str.str);
	}

#endif
	sys_free(mem_buffer);
	log_info("path-db-gen", "%s -> %s\n", in_path.str, out_file_path.str);
	return 1;
}
