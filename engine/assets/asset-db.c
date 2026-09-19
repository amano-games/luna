#include "asset-db.h"

#include "base/hash.h"
#include "engine/animation/animation.h"
#include "base/arr.h"
#include "lib/fnt/fnt.h"
#include "lib/bet/bet.h"
#include "base/dbg.h"
#include "base/ht.h"
#include "base/log.h"

// Intern path and return its path_table index. Index 0 is the empty sentinel.
static u32
path_id_intern(struct asset_db *db, str8 path)
{
	u32 id;

	asset_db_path_push(db, path, 0);
	id = ht_get_u32(&db->paths.ht, hash_fnv1a_str8(path));
	dbg_assert(id != 0);
	return id;
}

static str8
path_from_id(struct asset_db *db, u32 path_id)
{
	str8 res = {0};

	dbg_assert(path_id > 0);
	dbg_assert((ssize)path_id < arr_len(db->paths.arr));
	res = db->paths.arr[path_id];
	return res;
}

void
asset_db_ini(struct asset_db *db, struct asset_db_counts counts, struct alloc alloc)
{
	log_info(
		"Assets DB",
		"init paths=%u bytes=%u tex=%u clips=%u slices=%u fnt=%u snd=%u bet=%u",
		(uint)counts.paths,
		(uint)counts.path_bytes,
		(uint)counts.textures,
		(uint)counts.clips,
		(uint)counts.slices,
		(uint)counts.fonts,
		(uint)counts.snds,
		(uint)counts.bets);

	// Index 0 is the empty sentinel so ht miss (0) is never a real row.
	db->animations.ht   = ht_new_u32(ht_exp_from_count(counts.slices), alloc);
	db->animations.data = arr_new(alloc, db->animations.data, counts.clips + 1);
	db->animations.arr  = arr_new(alloc, db->animations.arr, counts.slices + 1);
	arr_push(db->animations.arr, (struct animation_slice){0});

	db->paths.ht   = ht_new_u32(ht_exp_from_count(counts.paths), alloc);
	db->paths.arr  = arr_new(alloc, db->paths.arr, counts.paths + 1);
	db->paths.tags = arr_new(alloc, db->paths.tags, counts.paths + 1);
	db->paths.data = arr_new(alloc, db->paths.data, counts.path_bytes ? counts.path_bytes : 1);
	arr_push(db->paths.arr, (str8){0});
	arr_push(db->paths.tags, 0);

	db->textures.ht  = ht_new_u32(ht_exp_from_count(counts.textures), alloc);
	db->textures.arr = arr_new(alloc, db->textures.arr, counts.textures + 1);
	arr_push(db->textures.arr, (struct asset_tex){0});

	db->atlases.ht  = ht_new_u32(ht_exp_from_count((usize)counts.textures), alloc);
	db->atlases.arr = arr_new(alloc, db->atlases.arr, counts.textures + 1);
	arr_push(db->atlases.arr, (struct tex_atlas){0});

	db->snds.ht  = ht_new_u32(ht_exp_from_count(counts.snds), alloc);
	db->snds.arr = arr_new(alloc, db->snds.arr, counts.snds + 1);
	arr_push(db->snds.arr, (struct asset_snd){0});

	db->fonts.ht  = ht_new_u32(ht_exp_from_count(counts.fonts), alloc);
	db->fonts.arr = arr_new(alloc, db->fonts.arr, counts.fonts + 1);
	arr_push(db->fonts.arr, (struct asset_fnt){0});

	db->bets.ht  = ht_new_u32(ht_exp_from_count(counts.bets), alloc);
	db->bets.arr = arr_new(alloc, db->bets.arr, counts.bets + 1);
	arr_push(db->bets.arr, (struct asset_bet){0});

	db->initialized = true;
}

struct asset_handle
asset_db_handle_from_path(str8 path, enum asset_type type)
{
	return (struct asset_handle){
		.path_hash = hash_fnv1a_str8(path),
		.type      = type,
	};
}

str8
asset_db_path_push(struct asset_db *db, str8 path, u64 tags)
{
	dbg_assert(path.size > 0);
	struct path_table *table = &db->paths;
	usize table_len          = arr_len(table->data);
	usize table_cap          = arr_cap(table->data);

	u64 key     = hash_fnv1a_str8(path);
	u32 value   = ht_get_u32(&table->ht, key);
	b32 has_key = value != 0;

	if(has_key) {
		table->tags[value] |= tags;
		return table->arr[value];
	}

	// Path bytes plus the trailing NUL pushed below.
	dbg_check(table_len + path.size + 1 <= table_cap, "AssetsDB", "Out of memory");
	dbg_check(arr_len(table->tags) < arr_cap(table->tags), "AssetsDB", "Out of memory");

	str8 res = (str8){.str = (u8 *)table->data + table_len, .size = path.size};
	value    = arr_len(table->arr);
	ht_set_u32(&table->ht, key, value);
	memcpy(table->data + arr_len(table->data), path.str, path.size);
	struct arr_header *header = arr_header(table->data);
	header->len += path.size;
	arr_push(table->data, '\0');
	arr_push(table->arr, res);
	arr_push(table->tags, tags);

	return res;

error:
	return (struct str8){0};
}

str8
asset_db_path_get(struct asset_db *db, struct asset_handle handle)
{
	str8 res                 = {0};
	struct path_table *table = &db->paths;
	u32 value                = ht_get_u32(&table->ht, handle.path_hash);

	dbg_check(value != 0, "AssetsDB", "missing path");
	res = table->arr[value];

error:
	return res;
}

b32
asset_db_is_path_tagged(struct asset_db *db, str8 path, u64 tag)
{
	b32 res = false;
	u32 id  = ht_get_u32(&db->paths.ht, hash_fnv1a_str8(path));

	if(id != 0) {
		res = (db->paths.tags[id] & tag) != 0;
	}

	return res;
}

void
asset_db_path_tag_set(struct asset_db *db, str8 path, u64 tag)
{
	u32 id = ht_get_u32(&db->paths.ht, hash_fnv1a_str8(path));

	dbg_check(id != 0, "AssetsDB", "missing path");
	db->paths.tags[id] |= tag;

error:;
}

void
asset_db_path_tag_clear(struct asset_db *db, str8 path, u64 tag)
{
	u32 id = ht_get_u32(&db->paths.ht, hash_fnv1a_str8(path));

	dbg_check(id != 0, "AssetsDB", "missing path");
	db->paths.tags[id] &= ~tag;

error:;
}

i32
asset_db_tex_push(struct asset_db *db, str8 path, struct tex tex)
{
	struct tex_table *table = &db->textures;
	ssize table_len         = arr_len(table->arr);
	ssize table_cap         = arr_cap(table->arr);

	// Can we add the string?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't push tex");
	u64 key                    = hash_fnv1a_str8(path);
	u32 value                  = ht_get_u32(&table->ht, key);
	b32 has_key                = value != 0;
	struct asset_tex asset_tex = {.path_id = path_id_intern(db, path), .tex = tex};

	if(has_key) {
		return value;
	} else {
		value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, asset_tex);
		return value;
	}

error:
	return 0;
}

struct asset_tex
asset_db_tex_get(struct asset_db *db, struct asset_handle handle)
{
	struct asset_tex res    = {0};
	struct tex_table *table = &db->textures;
	u32 value               = ht_get_u32(&table->ht, handle.path_hash);

	dbg_check(value != 0, "AssetsDB", "missing tex");
	res = table->arr[value];

error:
	return res;
}

struct str8
asset_db_tex_path_get(struct asset_db *db, u32 id)
{
	str8 res = {0};
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->textures.arr));
	res = path_from_id(db, db->textures.arr[id].path_id);
	return res;
}

i32
asset_db_tex_get_id(struct asset_db *db, struct asset_handle handle)
{
	struct tex_table *table = &db->textures;
	u32 res                 = ht_get_u32(&table->ht, handle.path_hash);
	return res;
}

struct asset_tex
asset_db_tex_get_by_id(struct asset_db *db, u32 id)
{
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->textures.arr));
	struct tex_table *table = &db->textures;
	struct asset_tex res    = table->arr[id];
	return res;
}

u32
asset_db_tex_atlas_push(struct asset_db *db, str8 path, struct tex_atlas atlas)
{
	struct tex_atlas_table *table = &db->atlases;
	usize table_len               = arr_len(table->arr);
	usize table_cap               = arr_cap(table->arr);
	u64 key                       = hash_fnv1a_str8(path);
	u32 value                     = ht_get_u32(&table->ht, key);
	b32 has_key                   = value != 0;

	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't push atlas");

	atlas.path_id = path_id_intern(db, path);

	if(has_key) {
		struct tex_atlas *row = &table->arr[value];

		if(row->cell_size.x && atlas.cell_size.x) {
			dbg_assert(row->cell_size.x == atlas.cell_size.x);
			dbg_assert(row->cell_size.y == atlas.cell_size.y);
		} else if(atlas.cell_size.x) {
			row->cell_size = atlas.cell_size;
		}

		return value;
	}

	value = table_len;
	ht_set_u32(&table->ht, key, value);
	arr_push(table->arr, atlas);
	return value;

error:
	return 0;
}

struct tex_atlas
asset_db_tex_atlas_get(struct asset_db *db, struct asset_handle handle)
{
	struct tex_atlas res          = {0};
	struct tex_atlas_table *table = &db->atlases;
	u32 value                     = ht_get_u32(&table->ht, handle.path_hash);

	if(value != 0) {
		res = table->arr[value];
	}

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
	struct animation_clip res    = {0};
	struct animation_slice slice = asset_db_animation_slice_get(db, handle);

	if(slice.clip != NULL) {
		dbg_assert(index < slice.size);
		struct animation_clip *clip = slice.clip + index;
		res                         = *clip;
	}

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

	u64 key     = hash_fnv1a_str8(path);
	u32 value   = ht_get_u32(&table->ht, key);
	b32 has_key = value != 0;

	if(has_key) {
		return value;
	} else {
		value = table_len;
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
	struct animation_slice res = {0};
	u32 index                  = ht_get_u32(&db->animations.ht, handle.path_hash);

	if(index != 0) {
		res = db->animations.arr[index];
	}

	return res;
}

u32
asset_db_snd_push(struct asset_db *db, str8 path, struct snd snd)
{
	u32 res                 = 0;
	struct snd_table *table = &db->snds;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the item?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't add snd");

	u64 key                    = hash_fnv1a_str8(path);
	u32 value                  = ht_get_u32(&table->ht, key);
	b32 has_key                = value != 0;
	struct asset_snd asset_snd = {.path_id = path_id_intern(db, path), .snd = snd};

	if(has_key) {
		res = value;
	} else {
		value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, asset_snd);
		res = value;
	}

error:
	return res;
}

struct asset_snd
asset_db_snd_get(struct asset_db *db, struct asset_handle handle)
{
	struct asset_snd res = {0};
	u32 index            = ht_get_u32(&db->snds.ht, handle.path_hash);

	dbg_check(index != 0, "AssetsDB", "missing snd");
	res = db->snds.arr[index];

error:
	return res;
}

str8
asset_db_snd_path_get(struct asset_db *db, u32 id)
{
	str8 res = {0};
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->snds.arr));
	res = path_from_id(db, db->snds.arr[id].path_id);
	return res;
}

u32
asset_db_snd_get_id(struct asset_db *db, struct asset_handle handle)
{
	u32 res = ht_get_u32(&db->snds.ht, handle.path_hash);
	return res;
}

struct asset_snd
asset_db_snd_get_by_id(struct asset_db *db, u32 id)
{
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->snds.arr));
	struct asset_snd res = db->snds.arr[id];
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

	u64 key                    = hash_fnv1a_str8(path);
	u32 value                  = ht_get_u32(&table->ht, key);
	b32 has_key                = value != 0;
	struct asset_fnt asset_fnt = {.path_id = path_id_intern(db, path), .fnt = fnt};

	if(has_key) {
		res = value;
	} else {
		value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, asset_fnt);
		res = value;
	}

error:
	return res;
}

struct asset_fnt
asset_db_fnt_get(struct asset_db *db, struct asset_handle handle)
{
	struct asset_fnt res = {0};
	u32 index            = ht_get_u32(&db->fonts.ht, handle.path_hash);

	dbg_check(index != 0, "AssetsDB", "missing fnt");
	res = db->fonts.arr[index];

error:
	return res;
}

u32
asset_db_fnt_get_id(struct asset_db *db, struct asset_handle handle)
{
	u32 res = ht_get_u32(&db->fonts.ht, handle.path_hash);
	return res;
}

struct asset_fnt
asset_db_fnt_get_by_id(struct asset_db *db, u32 id)
{
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->fonts.arr));
	struct asset_fnt res = db->fonts.arr[id];
	return res;
}

u32
asset_db_bet_push(
	struct asset_db *db,
	str8 path,
	struct bet bet)
{
	struct bet_table *table = &db->bets;
	usize table_len         = arr_len(table->arr);
	usize table_cap         = arr_cap(table->arr);

	// Can we add the item?
	dbg_check(table_len + 1 <= table_cap, "AssetsDB", "Can't push bet");

	u64 key                    = hash_fnv1a_str8(path);
	u32 value                  = ht_get_u32(&table->ht, key);
	b32 has_key                = value != 0;
	struct asset_bet asset_bet = {.path_id = path_id_intern(db, path), .bet = bet};

	if(has_key) {
		return value;
	} else {
		value = table_len;
		ht_set_u32(&table->ht, key, value);
		arr_push(table->arr, asset_bet);
		return value;
	}

error:
	return 0;
}

struct asset_bet_handle
asset_db_bet_handle_get(struct asset_db *db, struct asset_handle handle)
{
	u32 index                   = ht_get_u32(&db->bets.ht, handle.path_hash);
	struct asset_bet_handle res = (struct asset_bet_handle){.id = index};
	return res;
}

struct asset_bet
asset_db_bet_get(struct asset_db *db, struct asset_handle handle)
{
	struct asset_bet res = {0};
	u32 index            = ht_get_u32(&db->bets.ht, handle.path_hash);

	dbg_check(index != 0, "AssetsDB", "missing bet");
	res = db->bets.arr[index];

error:
	return res;
}

struct asset_bet
asset_db_bet_get_by_id(struct asset_db *db, u32 id)
{
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->bets.arr));
	struct asset_bet res = db->bets.arr[id];
	return res;
}

u32
asset_db_bet_get_id(struct asset_db *db, struct asset_handle handle)
{
	u32 res = ht_get_u32(&db->bets.ht, handle.path_hash);
	return res;
}

struct str8
asset_db_bet_path_get(struct asset_db *db, u32 id)
{
	str8 res = {0};
	dbg_assert(id > 0);
	dbg_assert((ssize)id < arr_len(db->bets.arr));
	res = path_from_id(db, db->bets.arr[id].path_id);
	return res;
}
