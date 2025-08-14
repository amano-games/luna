#include "asset-db.h"

#include "animation/animation.h"
#include "arr.h"
#include "fnt/fnt.h"
#include "audio/audio.h"
#include "bet/bet.h"
#include "bet/bet-ser.h"
#include "ht.h"
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
	usize snds_count,
	usize bets_count,
	struct alloc alloc)
{
	log_info("Assets DB", "init");
	usize paths_data_size = paths_count * 50;
	i32 exp               = 10;

	db->animations.ht   = ht_new_u32(exp, alloc);
	db->animations.data = arr_new(db->animations.data, clip_count + 1, alloc);
	db->animations.arr  = arr_new(db->animations.arr, slice_count + 1, alloc);
	arr_push(db->animations.arr, (struct animation_slice){0});

	db->paths.ht   = ht_new_u32(exp, alloc);
	db->paths.arr  = arr_new(db->paths.arr, paths_count + 1, alloc);
	db->paths.data = arr_new(db->paths.data, paths_data_size, alloc);
	arr_push(db->paths.arr, (str8){0});

	db->textures.ht  = ht_new_u32(exp, alloc);
	db->textures.arr = arr_new(db->textures.arr, textures_count + 1, alloc);
	arr_push(db->textures.arr, (struct tex){0});

	db->textures_info.ht  = ht_new_u32(exp, alloc);
	db->textures_info.arr = arr_new(db->textures_info.arr, textures_count + 1, alloc);
	arr_push(db->textures_info.arr, (struct tex_info){0});

	db->snds.ht  = ht_new_u32(exp, alloc);
	db->snds.arr = arr_new(db->snds.arr, snds_count + 1, alloc);
	arr_push(db->snds.arr, (struct snd){0});

	db->fonts.ht  = ht_new_u32(exp, alloc);
	db->fonts.arr = arr_new(db->fonts.arr, fonts_count + 1, alloc);
	arr_push(db->fonts.arr, (struct fnt){0});

	db->bets.ht  = ht_new_u32(exp, alloc);
	db->bets.arr = arr_new(db->bets.arr, bets_count + 1, alloc);
	arr_push(db->bets.arr, (struct asset_bet){0});
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
asset_db_path_push(struct asset_db *db, str8 path)
{
	dbg_assert(path.size > 0);
	struct path_table *table = &db->paths;
	usize table_len          = arr_len(table->data);
	usize table_cap          = arr_cap(table->data);

	// Can we add the string?
	dbg_check(table_len + path.size <= table_cap, "AssetsDB", "Out of memory");

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

error:
	return (struct str8){0};
}

str8
asset_db_path_get(struct asset_db *db, struct asset_handle handle)
{
	struct path_table *table = &db->paths;
	u32 value                = ht_get_u32(&table->ht, handle.path_hash);
	str8 res                 = table->arr[value];
	return res;
}

u32
asset_db_tex_push(struct asset_db *db, str8 path, struct tex tex)
{
	struct tex_table *table = &db->textures;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the string?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't push tex");
	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		return value;
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, tex);
		return value;
	}

error:
	return 0;
}

struct tex
asset_db_tex_get(struct asset_db *db, struct asset_handle handle)
{
	struct tex_table *table = &db->textures;
	u32 value               = ht_get_u32(&table->ht, handle.path_hash);
	struct tex res          = table->arr[value];
	return res;
}

struct str8
asset_db_tex_path_get(struct asset_db *db, u32 id)
{
	// WARN: I don't like this
	str8 res                = {0};
	u64 key                 = 0;
	struct tex_table *table = &db->textures;
	for(size i = 0; i < (size)arr_len(table->ht.ht); ++i) {
		if(table->ht.ht[i].value == id) {
			key = table->ht.ht[i].key;
			break;
		}
	}
	if(key != 0) {
		struct asset_handle handle = {.path_hash = key, .type = 0};
		res                        = asset_db_path_get(db, handle);
	}
	return res;
}

u32
asset_db_tex_get_id(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	struct tex_table *table = &db->textures;
	u32 res                 = ht_get_u32(&table->ht, handle.path_hash);
	TRACE_END();
	return res;
}

struct tex
asset_db_tex_get_by_id(struct asset_db *db, u32 id)
{
	TRACE_START(__func__);
	dbg_assert(id > 0);
	dbg_assert(id < arr_len(db->textures.arr));
	struct tex_table *table = &db->textures;
	struct tex res          = table->arr[id];
	TRACE_END();
	return res;
}

u32
asset_db_tex_info_push(struct asset_db *db, str8 path, struct tex_info info)
{
	struct tex_info_table *table = &db->textures_info;
	usize table_len              = arr_len(table->arr);
	usize table_cap              = arr_cap(table->arr);

	// Can we add the string?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't push tex info");

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

error:
	return 0;
}

struct tex_info
asset_db_tex_info_get(struct asset_db *db, struct asset_handle handle)
{
	struct tex_info_table *table = &db->textures_info;
	u32 value                    = ht_get_u32(&table->ht, handle.path_hash);
	struct tex_info res          = table->arr[value];
	return res;
}

u32
asset_db_animation_clip_push(struct asset_db *db, struct animation_clip clip)
{
	usize index = arr_len(db->animations.data);
	arr_push(db->animations.data, clip);
	animation_clip_init(&db->animations.data[index]);
	return index;
}

struct animation_clip
asset_db_animation_clip_get(struct asset_db *db, struct asset_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_clip res    = {0};
	struct animation_slice slice = asset_db_animation_slice_get(db, handle);

	if(slice.clip != NULL) {
		dbg_assert(index < slice.size);
		struct animation_clip *clip = slice.clip + index;
		res                         = *clip;
	}

	TRACE_END();
	return res;
}

struct animation_slice
asset_db_animation_slice_gen(struct asset_db *db, usize count)
{
	struct animation_slice res = {
		.clip = db->animations.data + arr_len(db->animations.data),
		.size = count,
	};

	return res;
}

u32
asset_db_animation_slice_push(struct asset_db *db, str8 path, struct animation_slice slice)
{
	struct animation_table *table = &db->animations;
	usize table_len               = arr_len(table->arr);
	usize table_cap               = arr_cap(table->arr);

	// Can we add the item?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Cant push animation slice");

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

error:
	return 0;
}

struct animation_slice
asset_db_animation_slice_get(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 index                  = ht_get_u32(&db->animations.ht, handle.path_hash);
	struct animation_slice res = db->animations.arr[index];
	TRACE_END();
	return res;
}

u32
asset_db_snd_push(struct asset_db *db, str8 path, struct snd snd)
{
	TRACE_START(__func__);
	u32 res                 = 0;
	struct snd_table *table = &db->snds;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the item?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't add snd");

	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		res = value;
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, snd);
		res = value;
	}

error:
	TRACE_END();
	return res;
}

struct snd
asset_db_snd_get(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 index      = ht_get_u32(&db->snds.ht, handle.path_hash);
	struct snd res = db->snds.arr[index];
	TRACE_END();
	return res;
}

u32
asset_db_snd_get_id(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 res = ht_get_u32(&db->snds.ht, handle.path_hash);
	TRACE_END();
	return res;
}

struct snd
asset_db_snd_get_by_id(struct asset_db *db, u32 id)
{
	dbg_assert(id > 0);
	dbg_assert(id < arr_len(db->snds.arr));
	TRACE_START(__func__);
	struct snd res = db->snds.arr[id];
	TRACE_END();
	return res;
}

u32
asset_db_fnt_push(struct asset_db *db, str8 path, struct fnt fnt)
{
	u32 res                 = 0;
	struct fnt_table *table = &db->fonts;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the item?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't add fnt");

	u64 key        = hash_string(path);
	u32 value      = ht_get_u32(&table->ht, key);
	bool32 has_key = value != 0;

	if(has_key) {
		res = value;
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, fnt);
		res = value;
	}

error:
	return res;
}

struct fnt
asset_db_fnt_get(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 index      = ht_get_u32(&db->fonts.ht, handle.path_hash);
	struct fnt res = db->fonts.arr[index];
	TRACE_END();
	return res;
}

u32
asset_db_fnt_get_id(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 res = ht_get_u32(&db->fonts.ht, handle.path_hash);
	TRACE_END();
	return res;
}

struct fnt
asset_db_fnt_get_by_id(struct asset_db *db, u32 id)
{
	dbg_assert(id > 0);
	dbg_assert(id < arr_len(db->fonts.arr));
	struct fnt res = db->fonts.arr[id];
	return res;
}

struct asset_bet_handle
asset_db_bet_load(
	struct asset_db *db,
	str8 path,
	struct alloc alloc,
	struct alloc scratch)
{
	struct bet_table *table = &db->bets;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the item?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't push bet");

	u64 key         = hash_string(path);
	u32 value       = ht_get_u32(&table->ht, key);
	bool32 has_key  = value != 0;
	usize timestamp = sys_file_modified(path);

	if(has_key) {
		return (struct asset_bet_handle){.id = value};
	} else {
		u32 value = table_len;
		ht_set_u32(&table->ht, key, value);
		struct bet bet             = bet_load(path, alloc, scratch);
		struct asset_bet asset_bet = {.bet = bet, .timestamp = timestamp};
		arr_push(table->arr, asset_bet);
		return (struct asset_bet_handle){.id = value};
	}

error:
	return (struct asset_bet_handle){0};
}

struct asset_bet_handle
asset_db_bet_handle_get(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 index                   = ht_get_u32(&db->bets.ht, handle.path_hash);
	struct asset_bet_handle res = (struct asset_bet_handle){.id = index};
	TRACE_END();
	return res;
}

struct bet *
asset_db_bet_get(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 index             = ht_get_u32(&db->bets.ht, handle.path_hash);
	struct asset_bet *res = db->bets.arr + index;
	TRACE_END();
	return &res->bet;
}

struct bet *
asset_db_bet_get_by_id(struct asset_db *db, struct asset_bet_handle handle)
{
	TRACE_START(__func__);
	dbg_assert(handle.id < arr_len(db->bets.arr));
	u32 index             = handle.id;
	struct asset_bet *res = db->bets.arr + index;
	TRACE_END();
	return &res->bet;
}

usize
asset_db_bet_get_timestamp_by_path(struct asset_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	u32 index             = ht_get_u32(&db->bets.ht, handle.path_hash);
	struct asset_bet *res = db->bets.arr + index;
	TRACE_END();
	return res->timestamp;
}

usize
asset_db_bet_get_timestamp_by_id(struct asset_db *db, struct asset_bet_handle handle)
{
	TRACE_START(__func__);
	dbg_assert(handle.id < arr_len(db->bets.arr));
	u32 index             = handle.id;
	struct asset_bet *res = db->bets.arr + index;
	TRACE_END();
	return res->timestamp;
}
