#include "asset-db-parser.h"

#include "assets.h"
#include "assets/asset-db.h"
#include "str.h"
#include "sys-io.h"
#include "arr.h"
#include "json.h"
#include "assets.h"
#include "str.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-utils.h"

str8
get_animation_json(str8 file_name, struct alloc alloc, struct alloc scratch)
{
	str8 path_name = str8_fmt_push(scratch, "%s%s", FILE_PATH_TEX, file_name.str);

	void *f = sys_file_open(path_name, SYS_FILE_R);
	if(f == NULL) {
		log_error("Animation DB", "failed to open db %s", path_name.str);
		BAD_PATH
	}
	struct sys_file_stats stats = sys_fstats(path_name);
	usize file_size             = stats.size;

	log_info("Animation", "animation db size: %d", (int)file_size);

	u8 *data = alloc.allocf(alloc.ctx, file_size);

	usize result = sys_file_read(f, data, file_size);

	sys_file_close(f);
	return (str8){.size = file_size, .str = data};
}

struct animation_track_res
handle_track(str8 json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	str8 bank_json  = {
		 .str  = json.str + root->start,
		 .size = root->end - root->start,
    };
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count                      = jsmn_parse(&parser, (const char *)bank_json.str, bank_json.size, NULL, 0);
	struct animation_track_res track_res = {.token_count = token_count};
	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("len")) == 0) {
			track_res.item.frames.len = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("frames")) == 0) {
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *item = &tokens[i + j + 2];

				i32 frame                      = json_parse_i32(json, item);
				track_res.item.frames.items[j] = frame;
			}
		}
	}
	return track_res;
}

struct animation_clip_res
handle_animation_clip(str8 json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	str8 bank_json  = {
		 .str  = json.str + root->start,
		 .size = root->end - root->start,
    };
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)bank_json.str, bank_json.size, NULL, 0);

	struct animation_clip_res data_res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("name")) == 0) {
		} else if(json_eq(json, key, str8_lit("count")) == 0) {
			data_res.item.count = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("frame_duration")) == 0) {
			data_res.item.frame_duration = json_parse_f32(json, value) / 1000;
		} else if(json_eq(json, key, str8_lit("scale")) == 0) {
			data_res.item.scale = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("tracks")) == 0) {
			assert(ARRLEN(data_res.item.tracks) == value->size);
			for(i32 j = 0; j < value->size; j++) {
				i32 track_index  = i + 2;
				jsmntok_t *track = &tokens[track_index];
				assert(track->type == JSMN_OBJECT);
				struct animation_track_res track_res = handle_track(json, tokens, track_index);
				data_res.item.tracks[j]              = track_res.item;
				i += track_res.token_count;
			}
		}
	}

	return data_res;
}

struct animation_slice_res
handle_animation_slice(str8 json, jsmntok_t *tokens, i32 index, struct asset_db *db)
{

	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	str8 item_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)item_json.str, item_json.size, NULL, 0);

	struct animation_slice_res res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("id")) == 0) {
		} else if(json_eq(json, key, str8_lit("path")) == 0) {
			str8 path = {.str = json.str + value->start, .size = value->end - value->start};
			res.path  = asset_db_push_path(db, path);
			assert(str8_match(path, res.path, 0));
		} else if(json_eq(json, key, str8_lit("len")) == 0) {
			res.item.size = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("clips")) == 0) {
			for(i32 j = 0; j < value->size; j++) {
				i32 clip_index  = i + 2;
				jsmntok_t *clip = &tokens[clip_index];
				assert(clip->type == JSMN_OBJECT);
				struct animation_clip_res res = handle_animation_clip(json, tokens, clip_index);
				asset_db_push_animation_clip(db, res.item);
				i += res.token_count;
			}
		}
	}

	return res;
}

struct asset_info_res
handle_asset_info(str8 json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	jsmn_parser parser;
	jsmn_init(&parser);

	str8 item_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};

	i32 token_count           = jsmn_parse(&parser, (char *)item_json.str, item_json.size, NULL, 0);
	struct asset_info_res res = {.token_count = token_count};
	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("cell_width")) == 0) {
			res.asset_info.cell_size.x = json_parse_i32(json, value);
			i++;
		} else if(json_eq(json, key, str8_lit("cell_height")) == 0) {
			res.asset_info.cell_size.y = json_parse_i32(json, value);
			i++;
		} else {
			log_info("Animation", "Unhandled key: %.*s", key->end - key->start, json.str + key->start);
		}
	}

	return res;
}

struct asset_res
handle_asset(str8 json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	jsmn_parser parser;
	jsmn_init(&parser);

	str8 item_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};

	i32 token_count = jsmn_parse(&parser, (char *)item_json.str, item_json.size, NULL, 0);

	struct asset_res res = {.token_count = token_count};
	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("path")) == 0) {
			res.path = (str8){.str = json.str + value->start, .size = value->end - value->start};
			i++;
		} else if(json_eq(json, key, str8_lit("info")) == 0) {
			i32 item_index        = i + 1;
			jsmntok_t *item_token = &tokens[item_index];
			assert(item_token->type == JSMN_OBJECT);
			struct asset_info_res item_res = handle_asset_info(json, tokens, item_index);
			res.asset_info                 = item_res.asset_info;
			i += item_res.token_count;
		} else {
			log_info("Animation", "Unhandled key handle assets: %.*s", key->end - key->start, json.str + key->start);
		}
	}
	return res;
}

struct assets_db_info_res
handle_info(str8 json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	str8 bank_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)bank_json.str, bank_json.size, NULL, 0);

	struct assets_db_info_res res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("version")) == 0) {
			res.item.version = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_clip_count")) == 0) {
			res.item.animation_clip_count = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_bank_count")) == 0) {
			res.item.animation_slice_count = json_parse_i32(json, value);
		}
	}

	return res;
}

void
asset_db_parse(struct asset_db *db, str8 file_name, struct alloc alloc, struct alloc scratch)
{
	log_info("Animation DB", "init: %s", file_name.str);

	str8 json = get_animation_json(file_name, scratch, scratch);
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);

	// Resset the parser
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	assert(json_res == token_count);

	for(i32 i = 1; i < token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("info")) == 0) {
			struct assets_db_info_res info_data = handle_info(json, tokens, i + 1);
			struct asset_db_info info           = info_data.item;
			assert(info.animation_clip_count <= arr_cap(db->animations.data));
			assert(info.animation_slice_count <= arr_cap(db->animations.arr));
			i += info_data.token_count;
		} else if(json_eq(json, key, str8_lit("assets")) == 0) {
			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index       = i + 2;
				struct asset_res res = handle_asset(json, tokens, item_index);
				str8 res_path        = asset_db_push_path(db, res.path);
				assert(str8_match(res_path, res.path, 0));
				asset_db_push_asset_info(db, res.path, res.asset_info);
				i += res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("animations")) == 0) {
			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index                    = i + 2;
				struct animation_clip *first_clip = &db->animations.data[arr_len(db->animations.data)];
				struct animation_slice_res res    = handle_animation_slice(json, tokens, item_index, db);
				res.item.clip                     = first_clip;
				asset_db_push_animation_slice(db, res.path, res.item);
				i += res.token_count;
			}
		}
	}
	log_info("Animation DB", "tokens:%" PRId32 " banks: %zu clips: %zu", token_count, arr_len(db->animations.arr), arr_len(db->animations.data));
}
