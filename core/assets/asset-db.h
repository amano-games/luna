#pragma once

#include "bet/bet.h"
#include "gfx/gfx.h"
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
	v2_i32 tex_size;
};

struct path_table {
	struct ht_u32 ht;
	str8 *arr;
	char *data;
};

struct tex_info_table {
	struct ht_u32 ht;
	struct tex_info *arr;
};

struct animation_table {
	struct ht_u32 ht;
	struct animation_slice *arr;
	struct animation_clip *data;
};

struct tex_table {
	struct ht_u32 ht;
	struct tex *arr;
};

struct fnt_table {
	struct ht_u32 ht;
	struct fnt *arr;
};

struct snd_table {
	struct ht_u32 ht;
	struct snd *arr;
};

struct asset_bet {
	struct bet bet;
	usize timestamp;
};

struct bet_table {
	struct ht_u32 ht;
	struct asset_bet *arr;
};

// [id] = index and count
struct asset_db {
	struct path_table paths;
	struct tex_info_table textures_info;
	struct tex_table textures;
	struct animation_table animations;
	struct fnt_table fonts;
	struct bet_table bets;
	struct snd_table snds;
};

void asset_db_init(struct asset_db *db, usize paths_count, usize textures_count, usize clip_count, usize slice_count, usize fonts_count, usize snds_count, usize bets_count, struct alloc alloc);
struct asset_handle asset_db_handle_from_path(str8 path, enum asset_type type);

str8 asset_db_path_push(struct asset_db *db, str8 path);
str8 asset_db_path_get(struct asset_db *db, struct asset_handle handle);

i32 asset_db_tex_push(struct asset_db *db, str8 path, struct tex tex);
i32 asset_db_tex_get_id(struct asset_db *db, struct asset_handle handle);
struct tex asset_db_tex_get(struct asset_db *db, struct asset_handle handle);
struct tex asset_db_tex_get_by_id(struct asset_db *db, i32 id);

i32 asset_db_tex_info_push(struct asset_db *db, str8 path, struct tex_info info);
struct tex_info asset_db_tex_info_get(struct asset_db *db, struct asset_handle handle);

i32 asset_db_animation_clip_push(struct asset_db *db, struct animation_clip clip);
struct animation_clip asset_db_animation_clip_get(struct asset_db *db, struct asset_handle handle, usize index);

struct animation_slice asset_db_animation_slice_gen(struct asset_db *db, usize count);
i32 asset_db_animation_slice_push(struct asset_db *db, str8 path, struct animation_slice slice);
struct animation_slice asset_db_animation_slice_get(struct asset_db *db, struct asset_handle handle);

i32 asset_db_snd_push(struct asset_db *db, str8 path, struct snd snd);
struct snd asset_db_snd_get(struct asset_db *db, struct asset_handle handle);
struct snd asset_db_snd_get_by_id(struct asset_db *db, i32 id);
i32 asset_db_snd_get_id(struct asset_db *db, struct asset_handle handle);

i32 asset_db_fnt_push(struct asset_db *db, str8 path, struct fnt fnt);
struct fnt asset_db_fnt_get(struct asset_db *db, struct asset_handle handle);
i32 asset_db_fnt_get_id(struct asset_db *db, struct asset_handle handle);
struct fnt asset_db_fnt_get_by_id(struct asset_db *db, i32 id);

struct asset_bet_handle asset_db_bet_load(struct asset_db *db, str8 path, struct alloc alloc, struct alloc scratch);
struct asset_bet_handle asset_db_bet_handle_get(struct asset_db *db, struct asset_handle handle);
struct bet *asset_db_bet_get(struct asset_db *db, struct asset_handle handle);
struct bet *asset_db_bet_get_by_id(struct asset_db *db, struct asset_bet_handle handle);

usize asset_db_bet_get_timestamp_by_path(struct asset_db *db, struct asset_handle handle);
usize asset_db_bet_get_timestamp_by_id(struct asset_db *db, struct asset_bet_handle handle);
