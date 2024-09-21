#pragma once

#include "str.h"
#include "sys-types.h"

#include "animation/animation.h"

struct asset_db_info {
	i32 version;
	usize animation_bank_count;
	usize animation_clip_count;
};

struct animation_data_slice_handle {
	u64 id;
};

struct animation_data_slice {
	u64 id;    // Hash that comes from the path
	u32 index; // where the slice starts in the DB
	u32 len;   // how many elements it has
};

// [id] = index and count
struct assets_db {
	struct animation_data_slice *slices;
	struct animation_data *clips;
};

void assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch);
void assets_db_init(struct assets_db *db, usize bank_count, usize clip_count, struct alloc alloc);
void assets_db_push_animation_data(struct assets_db *db, struct animation_data clip);
struct animation_data_slice_handle animation_db_bank_handle_from_path(str8 path);
void assets_db_push_animation_data_slice(struct assets_db *db, struct animation_data_slice slice);

struct animation_data_slice *animation_db_get_data_slice(struct assets_db *db, struct animation_data_slice_handle handle);
struct animation_data animation_db_get_clip(struct assets_db *db, struct animation_data_slice_handle handle, usize index);
