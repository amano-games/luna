#include "animation-db-parser.h"

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

string
get_animation_json(char *file_name, struct alloc *alloc)
{
	FILE_PATH_GEN(path_name, FILE_PATH_TEX, file_name);
	void *f = sys_file_open(path_name, SYS_FILE_R);
	if(f == NULL) {
		log_error("Animation DB", "failed to open db %s", path_name);
		BAD_PATH
	}
	struct sys_file_stats stats = sys_fstats(path_name);
	usize file_size             = stats.size;

	log_info("Animation", "animation db size: %d", (int)file_size);

	char *data = alloc->allocf(alloc->ctx, file_size);

	usize result = sys_file_read(f, data, file_size);

	sys_file_close(f);
	return (string){.len = file_size, .data = data};
}

struct animation_track_res
handle_track(string *json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root  = &tokens[index];
	string bank_json = {
		.data = json->data + root->start,
		.len  = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count                      = jsmn_parse(&parser, bank_json.data, bank_json.len, NULL, 0);
	struct animation_track_res track_res = {.token_count = token_count};
	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json->data, key, "len") == 0) {
			track_res.item.frames.len = json_parse_i32(json->data, value);
		} else if(json_eq(json->data, key, "frames") == 0) {
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *item = &tokens[i + j + 2];

				i32 frame                      = json_parse_i32(json->data, item);
				track_res.item.frames.items[j] = frame;
			}
		}
	}
	return track_res;
}

struct animation_data_res
handle_animation_data(string *json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root  = &tokens[index];
	string bank_json = {
		.data = json->data + root->start,
		.len  = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, bank_json.data, bank_json.len, NULL, 0);

	struct animation_data_res data_res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json->data, key, "name") == 0) {
		} else if(json_eq(json->data, key, "count") == 0) {
			data_res.item.count = json_parse_i32(json->data, value);
		} else if(json_eq(json->data, key, "frame_duration") == 0) {
			data_res.item.frame_duration = json_parse_f32(json->data, value) / 1000;
		} else if(json_eq(json->data, key, "scale") == 0) {
			data_res.item.scale = json_parse_f32(json->data, value);
		} else if(json_eq(json->data, key, "tracks") == 0) {
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
handle_animation_bank(string *json, jsmntok_t *tokens, i32 index, struct animation_data *arr)
{

	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	string bank_json = {
		.data = json->data + root->start,
		.len  = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, bank_json.data, bank_json.len, NULL, 0);

	struct animation_data_bank_res res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json->data, key, "id") == 0) {
		} else if(json_eq(json->data, key, "path") == 0) {
			// TODO: hash path
			string path = {.data = json->data + value->start, .len = value->end - value->start};
			u64 hash    = hash_string(path);
			res.item.id = hash;
		} else if(json_eq(json->data, key, "len") == 0) {
			res.item.len = json_parse_i32(json->data, value);
		} else if(json_eq(json->data, key, "clips") == 0) {
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
handle_info(string *json, jsmntok_t *tokens, i32 index)
{
	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	string bank_json = {
		.data = json->data + root->start,
		.len  = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, bank_json.data, bank_json.len, NULL, 0);

	struct animation_db_info_res res = {.token_count = token_count};

	for(i32 i = index + 1; i < index + token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json->data, key, "version") == 0) {
			res.item.version = json_parse_i32(json->data, value);
		} else if(json_eq(json->data, key, "animation_clip_count") == 0) {
			res.item.animation_clip_count = json_parse_i32(json->data, value);
		} else if(json_eq(json->data, key, "animation_bank_count") == 0) {
			res.item.animation_bank_count = json_parse_i32(json->data, value);
		}
	}

	return res;
}

void
parse_animation_db(struct animation_db *db, char *file_name, struct alloc *alloc, struct alloc *scratch)
{
	log_info("Animation DB", "init: %s", file_name);

	string json = get_animation_json(file_name, scratch);
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, json.data, json.len, NULL, 0);

	// Resset the parser
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	i32 json_res      = jsmn_parse(&parser, json.data, json.len, tokens, token_count);
	assert(json_res == token_count);

	struct animation_db_info info = {0};

	for(i32 i = 1; i < token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json.data, key, "animations") == 0) {
			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 bank_index                     = i + 2;
				usize bank_clip_index              = arr_len(db->clips);
				struct animation_data_bank_res res = handle_animation_bank(&json, tokens, bank_index, db->clips);
				res.item.index                     = bank_clip_index;
				arr_push(db->banks, res.item);
				i += res.token_count;
			}
		} else if(json_eq(json.data, key, "info") == 0) {
			struct animation_db_info_res info_res = handle_info(&json, tokens, i + 1);
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
