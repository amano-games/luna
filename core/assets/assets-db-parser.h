#pragma once

#include "sys-types.h"

#include "./assets-db.h"

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

struct animation_clips_slice_res {
	usize token_count;
	struct animation_clips_slice item;
};

struct assets_db_res {
	struct animation_clip *clips;
	struct animation_clips_slice *slices;
};

void asset_db_parser_do(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch);
