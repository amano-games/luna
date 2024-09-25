#include "assets-db.h"

#include "animation/animation.h"
#include "arr.h"
#include "ht.h"
#include "rand.h"
#include "str.h"
#include "sys-log.h"
#include "trace.h"

#include "./assets-db-parser.h"
#include <string.h>

void
assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch)
{
	asset_db_parser_do(db, file_name, alloc, scratch);

	// Set the tracks to the correct type
	for(usize i = 0; i < arr_len(db->clips); i++) {
		animation_clip_init(&db->clips[i]);
	}
}

void
assets_db_init(struct assets_db *db, usize clip_count, usize slice_count, struct alloc alloc)
{
	usize paths_size = sizeof(char) * 10000;

	db->clips  = arr_ini(clip_count, sizeof(*db->clips), alloc);
	db->slices = arr_ini(slice_count, sizeof(*db->slices), alloc);

	db->paths.ht   = ht_new_u32(10, alloc);
	db->paths.arr  = arr_ini(80, sizeof(*db->paths.arr), alloc);
	db->paths.data = arr_ini(10000, sizeof(*db->paths.data), alloc);
	arr_push(db->paths.arr, (str8){0});

	db->infos.ht  = ht_new_u32(10, alloc);
	db->infos.arr = arr_ini(80, sizeof(*db->infos.arr), alloc);
	arr_push(db->infos.arr, (struct asset_info){0});
}

struct asset_handle
assets_db_handle_from_path(str8 path, enum asset_type type)
{
	return (struct asset_handle){
		.path_hash = hash_string(path),
		.type      = type,
	};
}

void
assets_db_push_animation_clip(struct assets_db *db, struct animation_clip clip)
{
	arr_push(db->clips, clip);
	animation_clip_init(&db->clips[arr_len(db->clips) - 1]);
}

void
assets_db_push_animation_clip_slice(struct assets_db *db, struct animation_clips_slice slice)
{
	arr_push(db->slices, slice);
}

str8
assets_db_push_path(struct assets_db *db, str8 path)
{
	struct asset_path_table *table = &db->paths;
	usize table_len                = arr_len(table->data);
	usize table_cap                = arr_cap(table->data);

	// Can we add the string?
	if(table_len + path.size > table_cap) {
		BAD_PATH;
		return (str8){0}; // out of memory
	}

	u64 key        = hash_string(path);
	u32 value      = ht_has_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		return table->arr[value];
	}

	str8 res = (str8){.str = (u8 *)table->data + table_len, .size = path.size};
	value    = arr_len(table->arr);
	ht_set_u32(&table->ht, key, value);
	memcpy(table->data + arr_len(table->data), path.str, path.size);
	struct arr_header *header = arr_header(table->data);
	header->len += path.size;
	arr_push(table->data, '\0');
	arr_push(table->arr, res);

	return res;
}

str8
assets_db_get_path(struct assets_db *db, u64 hash)
{
	struct asset_path_table *table = &db->paths;
	u32 value                      = ht_has_u32(&table->ht, hash);
	str8 res                       = table->arr[value];
	return res;
}

i32
assets_db_push_asset_info(struct assets_db *db, str8 path, struct asset_info info)
{
	struct asset_info_table *table = &db->infos;
	usize table_len                = arr_len(table->arr);
	usize table_cap                = arr_cap(table->arr);

	// Can we add the string?
	if(table_len + 1 > table_cap) {
		BAD_PATH;
		return 0;
	}

	u64 key        = hash_string(path);
	u32 value      = ht_has_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		return value;
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, info);
		return value;
	}
}

struct asset_info
assets_db_get_asset_info(struct assets_db *db, u64 hash)
{
	struct asset_info_table *table = &db->infos;
	u32 value                      = ht_has_u32(&table->ht, hash);
	struct asset_info res          = table->arr[value];
	return res;
}

struct animation_clips_slice *
animation_db_get_clips_slice(struct assets_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	// TODO: make a hash table instead of this
	for(usize i = 0; i < arr_len(db->slices); ++i) {
		if(db->slices[i].path_hash == handle.path_hash) {
			TRACE_END();
			return &db->slices[i];
		}
	}
	TRACE_END();
	return NULL;
}

struct animation_clip
assets_db_get_animation_clip(struct assets_db *db, struct asset_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_clip res           = {0};
	struct animation_clips_slice *slice = animation_db_get_clips_slice(db, handle);

	if(slice != NULL) {
		usize abs_index = slice->index + index;
		assert(index <= slice->len);
		assert(abs_index <= arr_len(db->clips));
		res = db->clips[abs_index];
	}

	TRACE_END();
	return res;
}
