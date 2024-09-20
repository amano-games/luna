#pragma once

#include "str.h"
#include "sys-types.h"

#include "animation/animation.h"

struct asset_db_info {
	i32 version;
	usize animation_bank_count;
	usize animation_clip_count;
};

struct animation_data_bank_handle {
	u64 id;
};

struct animation_data_bank {
	u64 id;    // Hash that comes from the path
	u32 index; // where the slice starts in the DB
	u32 len;   // how many elements it has
};

// [id] = index and count
struct assets_db {
	struct animation_data_bank *banks;
	struct animation_data *clips;
};

void assets_db_parse(struct assets_db *db, str8 file_name, struct alloc *alloc, struct alloc *scratch);
struct animation_data_bank_handle animation_db_bank_handle_from_path(char *path);

struct animation_data_bank *animation_db_get_bank(struct assets_db *db, struct animation_data_bank_handle handle);
struct animation_data animation_db_get_clip(struct assets_db *db, struct animation_data_bank_handle handle, usize index);
