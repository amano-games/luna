#include "asset-db.h"

#include "animation/animation.h"
#include "arr.h"
#include "ht.h"
#include "str.h"
#include "sys-log.h"
#include "trace.h"

void
asset_db_init(
	struct asset_db *db,
	usize paths_count,
	usize textures_count,
	usize clip_count,
	usize slice_count,
	usize fonts_count,
	usize bets_count,
	struct alloc alloc)
{
	log_info("Assets DB", "init");
	usize paths_data_size = paths_count * 50;
	i32 exp               = 10;

	db->animations.ht   = ht_new_u32(exp, alloc);
	db->animations.data = arr_ini(clip_count, sizeof(*db->animations.data), alloc);
	db->animations.arr  = arr_ini(slice_count, sizeof(*db->animations.arr), alloc);
	arr_push(db->animations.arr, (struct animation_slice){0});

	db->assets_path.ht   = ht_new_u32(exp, alloc);
	db->assets_path.arr  = arr_ini(paths_count, sizeof(*db->assets_path.arr), alloc);
	db->assets_path.data = arr_ini(paths_data_size, sizeof(*db->assets_path.data), alloc);
	arr_push(db->assets_path.arr, (str8){0});

	db->textures_info.ht  = ht_new_u32(exp, alloc);
	db->textures_info.arr = arr_ini(textures_count, sizeof(*db->textures_info.arr), alloc);
	arr_push(db->textures_info.arr, (struct texture_info){0});

	db->fonts.ht  = ht_new_u32(exp, alloc);
	db->fonts.arr = arr_ini(fonts_count, sizeof(*db->fonts.arr), alloc);
	arr_push(db->fonts.arr, (struct fnt){0});

	db->bets.ht  = ht_new_u32(exp, alloc);
	db->bets.arr = arr_ini(bets_count, sizeof(*db->bets.arr), alloc);
	arr_push(db->bets.arr, (struct bet){0});
}

struct asset_handle
asset_db_handle_from_path(str8 path, enum asset_type type)
{
	return (struct asset_handle){
		.path_hash = hash_string(path),
		.type      = type,
	};
}

str8
asset_db_push_path(struct asset_db *db, str8 path)
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
asset_db_get_path(struct asset_db *db, struct asset_handle handle)
{
	struct asset_path_table *table = &db->assets_path;
	u32 value                      = ht_get_u32(&table->ht, handle.path_hash);
	str8 res                       = table->arr[value];
	return res;
}

i32
asset_db_push_asset_info(struct asset_db *db, str8 path, struct texture_info info)
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
asset_db_get_asset_info(struct asset_db *db, struct asset_handle handle)
{
	struct texture_info_table *table = &db->textures_info;
	u32 value                        = ht_get_u32(&table->ht, handle.path_hash);
	struct texture_info res          = table->arr[value];
	return res;
}

i32
asset_db_push_animation_clip(struct asset_db *db, struct animation_clip clip)
{
	usize index = arr_len(db->animations.data);
	arr_push(db->animations.data, clip);
	animation_clip_init(&db->animations.data[index]);
	return index;
}

struct animation_clip
asset_db_get_animation_clip(struct asset_db *db, struct asset_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_clip res    = {0};
	struct animation_slice slice = asset_db_get_animation_slice(db, handle);

	if(slice.clip != NULL) {
		assert(index < slice.size);
		struct animation_clip *clip = slice.clip + index;
		res                         = *clip;
	}

	TRACE_END();
	return res;
}

struct animation_slice
asset_db_gen_animation_slice(struct asset_db *db, usize count)
{
	struct animation_slice res = {
		.clip = db->animations.data + arr_len(db->animations.data),
		.size = count,
	};

	return res;
}

i32
asset_db_push_animation_slice(struct asset_db *db, str8 path, struct animation_slice slice)
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

struct animation_slice
asset_db_get_animation_slice(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	i32 index                  = ht_get_u32(&db->animations.ht, handle.path_hash);
	struct animation_slice res = db->animations.arr[index];
	TRACE_END();
	return res;
}

i32
asset_db_push_fnt(struct asset_db *db, str8 path, struct fnt fnt)
{
	struct fnt_table *table = &db->fonts;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

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
		arr_push(table->arr, fnt);
		return value;
	}
}

struct fnt
asset_db_get_fnt(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	i32 index      = ht_get_u32(&db->fonts.ht, handle.path_hash);
	struct fnt res = db->fonts.arr[index];
	TRACE_END();
	return res;
}

struct asset_bet_handle
asset_db_push_bet(struct asset_db *db, str8 path, struct bet bet)
{
	struct bet_table *table = &db->bets;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the item?
	if(table_len + 1 > table_cap) {
		BAD_PATH;
		return (struct asset_bet_handle){0};
	}

	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		return (struct asset_bet_handle){.id = value};
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, bet);
		return (struct asset_bet_handle){.id = value};
	}
}

struct asset_bet_handle
asset_db_get_bet_handle(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	i32 index                   = ht_get_u32(&db->bets.ht, handle.path_hash);
	struct asset_bet_handle res = (struct asset_bet_handle){.id = index};
	TRACE_END();
	return res;
}

struct bet *
asset_db_get_bet_by_path(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	i32 index       = ht_get_u32(&db->bets.ht, handle.path_hash);
	struct bet *res = db->bets.arr + index;
	TRACE_END();
	return res;
}

struct bet *
asset_db_get_bet_by_id(struct asset_db *db, struct asset_bet_handle handle)
{
	TRACE_START(__func__);
	assert(handle.id < arr_len(db->bets.arr));
	i32 index       = handle.id;
	struct bet *res = db->bets.arr + index;
	TRACE_END();
	return res;
}
