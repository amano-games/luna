#pragma once

#include "bet/bet.h"
#include "ht.h"
#include "sys-types.h"

#include "animation/animation.h"

enum asset_type {
	ASSET_TYPE_NONE,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_TEXTURE_INFO,
	ASSET_TYPE_ANIMATION_SLICE,
	ASSET_TYPE_ANIMATION_CLIP,
	ASSET_TYPE_SOUND,
	ASSET_TYPE_FONT,
	ASSET_TYPE_BET,
};

struct asset_handle {
	enum asset_type type;
	u64 path_hash;
};

struct asset_bet_handle {
	u8 id;
};

struct asset_db_info {
	i32 version;
	usize animation_slice_count;
	usize animation_clip_count;
};

struct animation_slice {
	struct animation_clip *clip;
	u32 size;
};

struct tex_info {
	u64 path_hash;
	v2_i32 cell_size;
};

struct asset_path_table {
	struct ht_u32 ht;
	str8 *arr;
	char *data;
};

struct texture_info_table {
	struct ht_u32 ht;
	struct tex_info *arr;
};

struct animation_table {
	struct ht_u32 ht;
	struct animation_slice *arr;
	struct animation_clip *data;
};

struct texture_table {
	struct ht_u32 ht;
	struct asset_tex *arr;
};

struct fnt_table {
	struct ht_u32 ht;
	struct fnt *arr;
};

struct bet_table {
	struct ht_u32 ht;
	struct bet *arr;
};

// [id] = index and count
struct asset_db {
	struct asset_path_table assets_path;
	struct texture_info_table textures_info;
	struct animation_table animations;
	struct texture_table textures;
	struct fnt_table fonts;
	struct bet_table bets;
};

void asset_db_init(struct asset_db *db, usize paths_count, usize textures_count, usize clip_count, usize slice_count, usize fonts_count, usize bets_count, struct alloc alloc);

struct asset_handle asset_db_handle_from_path(str8 path, enum asset_type type);

str8 asset_db_push_path(struct asset_db *db, str8 path);
str8 asset_db_get_path(struct asset_db *db, struct asset_handle handle);

i32 asset_db_push_asset_info(struct asset_db *db, str8 path, struct tex_info info);
struct tex_info asset_db_get_asset_info(struct asset_db *db, struct asset_handle handle);

i32 asset_db_push_animation_clip(struct asset_db *db, struct animation_clip clip);
struct animation_clip asset_db_get_animation_clip(struct asset_db *db, struct asset_handle handle, usize index);

struct animation_slice asset_db_gen_animation_slice(struct asset_db *db, usize count);
i32 asset_db_push_animation_slice(struct asset_db *db, str8 path, struct animation_slice slice);
struct animation_slice asset_db_get_animation_slice(struct asset_db *db, struct asset_handle handle);

// TODO: Maybe for fonts we don't need that a hash table, how many fonts do we have ? 10? wouldn't it be easier to use an array and a custom handle that it's the index of the array and we save one lookup
i32 asset_db_load_fnt(struct asset_db *db, str8 path, struct fnt *fnt);
i32 asset_db_push_fnt(struct asset_db *db, str8 path, struct fnt fnt);
struct fnt asset_db_get_fnt(struct asset_db *db, struct asset_handle handle);

struct asset_bet_handle asset_db_load_bet(struct asset_db *db, str8 path, struct alloc alloc, struct alloc scratch);
struct asset_bet_handle asset_db_get_bet_handle(struct asset_db *db, struct asset_handle handle);
struct bet *asset_db_get_bet_by_path(struct asset_db *db, struct asset_handle handle);
struct bet *asset_db_get_bet_by_id(struct asset_db *db, struct asset_bet_handle handle);
