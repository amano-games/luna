#pragma once

#include "ht.h"
#include "str.h"
#include "sys-types.h"

#include "animation/animation.h"

struct asset_path_table {
	struct ht_u32 ht;
	str8 *arr;
	char *data;
};

struct asset_info_table {
	struct ht_u32 ht;
	struct asset_info *arr;
};

enum asset_type {
	ASSET_TYPE_NONE,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_ANIMATION_SLICE,
	ASSET_TYPE_ANIMATION,
};

struct asset_handle {
	enum asset_type type;
	u64 path_hash;
};

struct asset_db_info {
	i32 version;
	usize animation_slice_count;
	usize animation_clip_count;
};

struct animation_clips_slice {
	u64 path_hash; // Hash that comes from the path
	u32 index;     // where the slice starts in the DB
	u32 len;       // how many elements it has
};

struct asset_info {
	u64 path_hash;
	v2_i32 cell_size;
};

// [id] = index and count
struct assets_db {
	struct asset_path_table paths;
	struct asset_info_table infos;

	struct asset_tex *tex;
	struct animation_clips_slice *slices;
	struct animation_clip *clips;
};

void assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch);
void assets_db_init(struct assets_db *db, usize clip_count, usize slice_count, struct alloc alloc);

struct asset_handle assets_db_handle_from_path(str8 path, enum asset_type type);

str8 assets_db_push_path(struct assets_db *db, str8 path);
str8 assets_db_get_path(struct assets_db *db, u64 hash);

i32 assets_db_push_asset_info(struct assets_db *db, str8 path, struct asset_info info);
struct asset_info assets_db_get_asset_info(struct assets_db *db, u64 hash);

void assets_db_push_animation_clip(struct assets_db *db, struct animation_clip clip);
struct animation_clip assets_db_get_animation_clip(struct assets_db *db, struct asset_handle handle, usize index);

void assets_db_push_animation_clip_slice(struct assets_db *db, struct animation_clips_slice slice);
struct animation_clips_slice *animation_db_get_clips_slice(struct assets_db *db, struct asset_handle handle);
