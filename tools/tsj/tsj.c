#include "tsj.h"

#include "base/arr.h"
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
#include "sys/sys.h"

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
	enum path_style style = path_style_relative;
	str8 res              = {0};
	str8 base_dir         = str8_chop_last_slash(in_path);
	str8 path_with_dots   = str8_fmt_push(scratch, "%.*s/%.*s", (i32)base_dir.size, base_dir.str, (i32)path.size, path.str);
	res                   = path_resolve_dots(scratch, path_with_dots, style, scratch);
	res                   = make_file_name_with_ext(alloc, res, str8_lit("tex"));
	str8 prefix           = str8_lit("src/");
	res                   = str8_skip(res, prefix.size);
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
			i32 i                 = 0;
			for(struct str8_node *n = list.first; n != 0; n = n->next) {
				// NOTE: we remove 1 because in tiled we specify them starting with 1 instead of 0
				res.track.frames.items[i++] = str8_to_i32(n->str) - 1;
			}

			res.track.frames.len = i;
			res.track.frames.cap = i;
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
			str8 path      = json_str8_cpy_push(json, value, scratch);
			res.asset.path = tsj_handle_path(path, in_path, alloc, scratch);
		} else if(json_eq(json, key, str8_lit("width")) == 0) {
			res.asset.info.cell_size.x = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("height")) == 0) {
			res.asset.info.cell_size.y = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("imagewidth")) == 0) {
			res.asset.info.tex_size.x = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("imageheight")) == 0) {
			res.asset.info.tex_size.y = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("properties")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.asset.clips = arr_new(res.asset.clips, value->size, alloc);
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
					arr_push(res.asset.clips, item_res.clip);
				}
				i += item_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("objectgroup")) == 0) {
			usize skip_count = json_obj_count(json, value);
			i += skip_count;
		} else {
		}
	}

	if(res.asset.info.cell_size.x == 0) {
		res.asset.info.cell_size.x = res.asset.info.tex_size.x;
	}
	if(res.asset.info.cell_size.y == 0) {
		res.asset.info.cell_size.y = res.asset.info.tex_size.y;
	}

	return res;
}

struct ani_db
tsj_handle_json(
	str8 in_path,
	str8 json,
	struct alloc alloc,
	struct alloc scratch)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(tokens, token_count, scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_assert(json_res == token_count);

	jsmntok_t root = tokens[0];

	dbg_assert(root.type == JSMN_OBJECT);

	struct ani_db res = {0};

	for(usize i = 0; i < (usize)token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("tilecount")) == 0) {
			usize count = json_parse_i32(json, value);
			res.assets  = arr_new(res.assets, count, alloc);
		} else if(json_eq(json, key, str8_lit("tiles")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			dbg_assert(arr_cap(res.assets) == (usize)value->size);
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
				arr_push(res.assets, tile_res.asset);

				i += tile_res.token_count;
			}
		}
	}

	dbg_assert(arr_len(res.assets) == arr_cap(res.assets));
	for(usize i = 0; i < arr_len(res.assets); ++i) {
		struct ani_db_asset asset = res.assets[i];
		usize clip_count          = arr_len(asset.clips);
		res.clip_count += clip_count;
		if(clip_count > 0) { res.bank_count++; }
	}

	return res;
}

i32
handle_tsj(str8 in_path, str8 out_path, struct alloc scratch)
{
	i32 res = 0;

	usize mem_size = MKILOBYTE(100);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	dbg_assert(mem_buffer != NULL);
	struct marena marean = {0};
	marena_init(&marean, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marean);

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(ANIMATION_DB_EXT));

	str8 json = {0};
	json_load(in_path, scratch, &json);
	struct ani_db db = tsj_handle_json(in_path, json, alloc, scratch);

	// Fill all empty tracks with a sequence 0,1,2,3...num of cells
	for(usize i = 0; i < arr_len(db.assets); ++i) {
		struct ani_db_asset *asset = db.assets + i;
		for(usize j = 0; j < arr_len(asset->clips); ++j) {
			struct animation_clip *clip = asset->clips + j;
			if(clip->tracks[0].frames.len == 0) {
				usize cells_count          = asset->info.tex_size.x / asset->info.cell_size.x;
				clip->tracks[0].frames.len = cells_count;
				for(usize k = 0; k < cells_count; ++k) {
					clip->tracks[0].frames.items[k] = k;
				}
			}
		}
	}

	void *out_file;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("ani-db-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	struct ser_writer w = {.f = out_file};
	ani_db_write(&w, db);

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
	log_info("ani-db-gen", "%s -> %s\n", in_path.str, out_file_path.str);
	return 1;
}
