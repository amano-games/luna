#pragma once

#include "sys-types.h"

#include "./asset-db.h"

#include "animation/animation.h"

struct assets_db_info_res {
	usize token_count;
	struct asset_db_info item;
};

struct animation_track_res {
	usize token_count;
	struct animation_track item;
};

struct animation_clip_res {
	usize token_count;
	struct animation_clip item;
};

struct animation_slice_res {
	usize token_count;
	str8 path;
	struct animation_slice item;
};

struct asset_res {
	usize token_count;
	struct str8 path;
	struct tex_info asset_info;
};

struct asset_info_res {
	usize token_count;
	struct tex_info asset_info;
};

struct assets_db_res {
	struct animation_clip *clips;
	struct animation_slice *slices;
};

void asset_db_parse(struct asset_db *db, str8 file_name, struct alloc alloc, struct alloc scratch);
