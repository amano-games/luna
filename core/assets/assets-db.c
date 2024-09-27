#include "assets-db.h"

#include "animation/animation.h"
#include "arr.h"
#include "ht.h"
#include "str.h"
#include "trace.h"

#include "./assets-db-parser.h"

void
assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch)
{
	asset_db_parser_do(db, file_name, alloc, scratch);
}

void
assets_db_init(
	struct assets_db *db,
	usize paths_count,
	usize textures_count,
	usize clip_count,
	usize slice_count,
	struct alloc alloc)
{
	usize paths_data_size = paths_count * 50;
	i32 exp               = 10;

	db->animations.ht   = ht_new_u32(exp, alloc);
	db->animations.data = arr_ini(clip_count, sizeof(*db->animations.data), alloc);
	db->animations.arr  = arr_ini(slice_count, sizeof(*db->animations.arr), alloc);
	arr_push(db->animations.arr, (struct animation_clips_slice){0});

	db->assets_path.ht   = ht_new_u32(exp, alloc);
	db->assets_path.arr  = arr_ini(paths_count, sizeof(*db->assets_path.arr), alloc);
	db->assets_path.data = arr_ini(paths_data_size, sizeof(*db->assets_path.data), alloc);
	arr_push(db->assets_path.arr, (str8){0});

	db->textures_info.ht  = ht_new_u32(exp, alloc);
	db->textures_info.arr = arr_ini(textures_count, sizeof(*db->textures_info.arr), alloc);
	arr_push(db->textures_info.arr, (struct texture_info){0});
}

struct asset_handle
assets_db_handle_from_path(str8 path, enum asset_type type)
{
	return (struct asset_handle){
		.path_hash = hash_string(path),
		.type      = type,
	};
}

str8
assets_db_push_path(struct assets_db *db, str8 path)
{
	struct asset_path_table *table = &db->assets_path;
	usize table_len                = arr_len(table->data);
	usize table_cap                = arr_cap(table->data);

	// Can we add the string?
	if(table_len + path.size > table_cap) {
		BAD_PATH;
		return (str8){0}; // out of memory
	}

	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
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
assets_db_get_path(struct assets_db *db, struct asset_handle handle)
{
	struct asset_path_table *table = &db->assets_path;
	u32 value                      = ht_get_u32(&table->ht, handle.path_hash);
	str8 res                       = table->arr[value];
	return res;
}

i32
assets_db_push_asset_info(struct assets_db *db, str8 path, struct texture_info info)
{
	struct texture_info_table *table = &db->textures_info;
	usize table_len                  = arr_len(table->arr);
	usize table_cap                  = arr_cap(table->arr);

	// Can we add the string?
	if(table_len + 1 > table_cap) {
		BAD_PATH;
		return 0;
	}

	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
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

struct texture_info
assets_db_get_asset_info(struct assets_db *db, struct asset_handle handle)
{
	struct texture_info_table *table = &db->textures_info;
	u32 value                        = ht_get_u32(&table->ht, handle.path_hash);
	struct texture_info res          = table->arr[value];
	return res;
}

i32
assets_db_push_animation_clip(struct assets_db *db, struct animation_clip clip)
{
	usize index = arr_len(db->animations.data);
	arr_push(db->animations.data, clip);
	animation_clip_init(&db->animations.data[index]);
	return index;
}

struct animation_clip
assets_db_get_animation_clip(struct assets_db *db, struct asset_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_clip res          = {0};
	struct animation_clips_slice slice = assets_db_get_animation_clips_slice(db, handle);

	if(slice.clip != NULL) {
		assert(index < slice.size);
		struct animation_clip *clip = slice.clip + index;
		res                         = *clip;
	}

	TRACE_END();
	return res;
}

i32
assets_db_push_animation_clip_slice(struct assets_db *db, str8 path, struct animation_clips_slice slice)
{
	struct animation_table *table = &db->animations;
	usize table_len               = arr_len(table->arr);
	usize table_cap               = arr_cap(table->arr);

	// Can we add the item?
	if(table_len + 1 > table_cap) {
		BAD_PATH;
		return 0;
	}

	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		return value;
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, slice);
		return value;
	}
}

struct animation_clips_slice
assets_db_get_animation_clips_slice(struct assets_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	i32 index                        = ht_get_u32(&db->animations.ht, handle.path_hash);
	struct animation_clips_slice res = db->animations.arr[index];
	TRACE_END();
	return res;
}
