#include "assets-db-parser.h"

#include "assets.h"
#include "ht.h"
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
get_animation_json(str8 file_name, struct alloc *alloc, struct alloc *scratch)
{
	str8 path_name = str8_fmt_push(scratch, "%s.%s", file_name, FILE_PATH_TEX);

	void *f = sys_file_open(path_name, SYS_FILE_R);
	if(f == NULL) {
		log_error("Animation DB", "failed to open db %s", path_name.str);
		BAD_PATH
	}
	struct sys_file_stats stats = sys_fstats(path_name);
	usize file_size             = stats.size;

	log_info("Animation", "animation db size: %d", (int)file_size);

	u8 *data = alloc->allocf(alloc->ctx, file_size);

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

struct animation_data_res
handle_animation_data(str8 json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	str8 bank_json  = {
		 .str  = json.str + root->start,
		 .size = root->end - root->start,
    };
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)bank_json.str, bank_json.size, NULL, 0);

	struct animation_data_res data_res = {.token_count = token_count};

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

// TODO: allocate and push to the db
struct animation_data_bank_res
handle_animation_bank(str8 json, jsmntok_t *tokens, i32 index, struct animation_data *arr)
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

	struct animation_data_bank_res res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("id")) == 0) {
		} else if(json_eq(json, key, str8_lit("path")) == 0) {
			str8 path   = {.str = json.str + value->start, .size = value->end - value->start};
			u64 hash    = hash_string(path);
			res.item.id = hash;
		} else if(json_eq(json, key, str8_lit("len")) == 0) {
			res.item.len = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("clips")) == 0) {
			for(i32 j = 0; j < value->size; j++) {
				i32 clip_index  = i + 2;
				jsmntok_t *clip = &tokens[clip_index];
				assert(clip->type == JSMN_OBJECT);
				struct animation_data_res res = handle_animation_data(json, tokens, clip_index);
				arr_push(arr, res.item);
				i += res.token_count;
			}
		}
	}

	return res;
}

struct animation_db_info_res
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

	struct animation_db_info_res res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("version")) == 0) {
			res.item.version = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_clip_count")) == 0) {
			res.item.animation_clip_count = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_bank_count")) == 0) {
			res.item.animation_bank_count = json_parse_i32(json, value);
		}
	}

	return res;
}

void
asset_db_parser_do(struct assets_db *db, str8 file_name, struct alloc *alloc, struct alloc *scratch)
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

	struct asset_db_info info = {0};

	for(i32 i = 1; i < token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("animations")) == 0) {
			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 bank_index                     = i + 2;
				usize bank_clip_index              = arr_len(db->clips);
				struct animation_data_bank_res res = handle_animation_bank(json, tokens, bank_index, db->clips);
				res.item.index                     = bank_clip_index;
				arr_push(db->banks, res.item);
				i += res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("info")) == 0) {
			struct animation_db_info_res info_res = handle_info(json, tokens, i + 1);
			info                                  = info_res.item;
			db->clips                             = arr_ini(info.animation_clip_count, sizeof(*db->clips), alloc);
			db->banks                             = arr_ini(info.animation_bank_count, sizeof(*db->banks), alloc);
			i += info_res.token_count;
		}
	}

	assert(arr_len(db->clips) == info.animation_clip_count);
	assert(arr_len(db->banks) == info.animation_bank_count);

	log_info("Animation DB", "tokens:%" PRId32 " banks: %zu clips: %zu", token_count, info.animation_bank_count, info.animation_clip_count);
}
