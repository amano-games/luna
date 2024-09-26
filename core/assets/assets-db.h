#pragma once

#include "ht.h"
#include "str.h"
#include "sys-types.h"

#include "animation/animation.h"

enum asset_type {
	ASSET_TYPE_NONE,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_TEXTURE_INFO,
	ASSET_TYPE_ANIMATION_SLICE,
	ASSET_TYPE_ANIMATION_CLIP,
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
	struct animation_clip *clip;
	u32 size;
};

struct texture_info {
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
	struct texture_info *arr;
};

struct animation_table {
	struct ht_u32 ht;
	struct animation_clips_slice *arr;
	struct animation_clip *data;
};

struct texture_table {
	struct ht_u32 ht;
	struct asset_tex *arr;
};

// [id] = index and count
struct assets_db {
	struct asset_path_table assets_path;
	struct texture_info_table textures_info;
	struct animation_table animations;
	struct texture_table textures;
};

void assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch);
void assets_db_init(struct assets_db *db, usize paths_count, usize textures_count, usize clip_count, usize slice_count, struct alloc alloc);

struct asset_handle assets_db_handle_from_path(str8 path, enum asset_type type);

str8 assets_db_push_path(struct assets_db *db, str8 path);
str8 assets_db_get_path(struct assets_db *db, struct asset_handle handle);

i32 assets_db_push_asset_info(struct assets_db *db, str8 path, struct texture_info info);
struct texture_info assets_db_get_asset_info(struct assets_db *db, struct asset_handle handle);

i32 assets_db_push_animation_clip(struct assets_db *db, struct animation_clip clip);
struct animation_clip assets_db_get_animation_clip(struct assets_db *db, struct asset_handle handle, usize index);

i32 assets_db_push_animation_clip_slice(struct assets_db *db, str8 path, struct animation_clips_slice slice);
struct animation_clips_slice animation_db_get_clips_slice(struct assets_db *db, struct asset_handle handle);
