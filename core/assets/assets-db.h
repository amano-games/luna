#pragma once

#include "str.h"
#include "sys-types.h"

#include "animation/animation.h"

enum asset_type {
	ASSET_TYPE_NONE,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_ANIMATION,
};

struct asset_handle {
	enum asset_type type;
	u64 path_hash;
};

struct asset_db_info {
	i32 version;
	usize animation_bank_count;
	usize animation_clip_count;
};

struct animation_clips_slice {
	u64 path_hash; // Hash that comes from the path
	u32 index;     // where the slice starts in the DB
	u32 len;       // how many elements it has
};

// [id] = index and count
struct assets_db {
	struct animation_clips_slice *slices;
	struct animation_clip *clips;
};

void assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch);
void assets_db_init(struct assets_db *db, usize bank_count, usize clip_count, struct alloc alloc);
void assets_db_push_animation_clip(struct assets_db *db, struct animation_clip clip);
struct asset_handle assets_db_handle_from_path(str8 path, enum asset_type type);
void assets_db_push_animation_clip_slice(struct assets_db *db, struct animation_clips_slice slice);

struct animation_clips_slice *animation_db_get_data_slice(struct assets_db *db, struct asset_handle handle);
struct animation_clip assets_db_get_animation_clip(struct assets_db *db, struct asset_handle handle, usize index);
