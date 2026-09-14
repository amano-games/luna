#pragma once

#include "engine/audio/snd.h"
#include "lib/bet/bet.h"
#include "lib/fnt/fnt.h"
#include "base/ht.h"
#include "base/types.h"

#include "engine/animation/animation.h"
#include "lib/tex/tex.h"

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

struct asset_tex_info {
	u32 path_id;
	v2_i32 cell_size;
	v2_i32 tex_size;
};

// path_id is path_table.arr index so id -> str8 skips the hash table.
struct asset_tex {
	u32 path_id;
	struct tex tex;
};

struct asset_snd {
	u32 path_id;
	struct snd snd;
};

struct asset_fnt {
	u32 path_id;
	struct fnt fnt;
};

struct asset_bet {
	u32 path_id;
	struct bet bet;
};

struct path_table {
	struct ht_u32 ht;
	str8 *arr;
	u64 *tags;
	char *data;
};

struct tex_info_table {
	struct ht_u32 ht;
	struct asset_tex_info *arr;
};

struct animation_table {
	struct ht_u32 ht;
	struct animation_slice *arr;
	struct animation_clip *data;
};

struct tex_table {
	struct ht_u32 ht;
	struct asset_tex *arr;
};

struct fnt_table {
	struct ht_u32 ht;
	struct asset_fnt *arr;
};

struct snd_table {
	struct ht_u32 ht;
	struct asset_snd *arr;
};

struct bet_table {
	struct ht_u32 ht;
	struct asset_bet *arr;
};

struct asset_db {
	b32 initialized;
	struct path_table paths;
	struct tex_info_table textures_info;
	struct tex_table textures;
	struct animation_table animations;
	struct fnt_table fonts;
	struct bet_table bets;
	struct snd_table snds;
};

struct asset_db_counts {
	ssize paths;
	ssize path_bytes;
	ssize textures;
	ssize clips;
	ssize slices;
	ssize fonts;
	ssize snds;
	ssize bets;
};

void asset_db_ini(struct asset_db *db, struct asset_db_counts counts, struct alloc alloc);
struct asset_handle asset_db_handle_from_path(str8 path, enum asset_type type);

str8 asset_db_path_push(struct asset_db *db, str8 path, u64 tags);
str8 asset_db_path_get(struct asset_db *db, struct asset_handle handle);
b32 asset_db_is_path_tagged(struct asset_db *db, str8 path, u64 tag);
void asset_db_path_tag_set(struct asset_db *db, str8 path, u64 tag);
void asset_db_path_tag_clear(struct asset_db *db, str8 path, u64 tag);

i32 asset_db_tex_push(struct asset_db *db, str8 path, struct tex tex);
i32 asset_db_tex_get_id(struct asset_db *db, struct asset_handle handle);
struct asset_tex asset_db_tex_get(struct asset_db *db, struct asset_handle handle);
struct asset_tex asset_db_tex_get_by_id(struct asset_db *db, u32 id);
struct str8 asset_db_tex_path_get(struct asset_db *db, u32 id);

u32 asset_db_tex_info_push(struct asset_db *db, str8 path, struct asset_tex_info info);
struct asset_tex_info asset_db_tex_info_get(struct asset_db *db, struct asset_handle handle);

u32 asset_db_animation_clip_push(struct asset_db *db, struct animation_clip clip);
struct animation_clip asset_db_animation_clip_get(struct asset_db *db, struct asset_handle handle, usize index);

struct animation_slice asset_db_animation_slice_gen(struct asset_db *db, usize count);
u32 asset_db_animation_slice_push(struct asset_db *db, str8 path, struct animation_slice slice);
struct animation_slice asset_db_animation_slice_get(struct asset_db *db, struct asset_handle handle);

u32 asset_db_snd_push(struct asset_db *db, str8 path, struct snd snd);
struct asset_snd asset_db_snd_get(struct asset_db *db, struct asset_handle handle);
struct asset_snd asset_db_snd_get_by_id(struct asset_db *db, u32 id);
u32 asset_db_snd_get_id(struct asset_db *db, struct asset_handle handle);
str8 asset_db_snd_path_get(struct asset_db *db, u32 id);

u32 asset_db_fnt_push(struct asset_db *db, str8 path, struct fnt fnt);
u32 asset_db_fnt_get_id(struct asset_db *db, struct asset_handle handle);
struct asset_fnt asset_db_fnt_get(struct asset_db *db, struct asset_handle handle);
struct asset_fnt asset_db_fnt_get_by_id(struct asset_db *db, u32 id);

u32 asset_db_bet_push(struct asset_db *db, str8 path, struct bet bet);
struct asset_bet_handle asset_db_bet_handle_get(struct asset_db *db, struct asset_handle handle);
struct asset_bet asset_db_bet_get(struct asset_db *db, struct asset_handle handle);
struct asset_bet asset_db_bet_get_by_id(struct asset_db *db, u32 id);
u32 asset_db_bet_get_id(struct asset_db *db, struct asset_handle handle);
struct str8 asset_db_bet_path_get(struct asset_db *db, u32 id);
