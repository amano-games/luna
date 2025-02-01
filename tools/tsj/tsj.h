#pragma once

#include "animation/animation.h"
#include "animation/animation-db.h"

struct tsj_tile_res {
	usize token_count;
	struct ani_db_asset asset;
};

struct tsj_property_res {
	usize token_count;
	struct animation_clip clip;
};

struct tsj_animation_res {
	usize token_count;
	struct animation_clip clip;
};

struct tsj_track_res {
	usize token_count;
	struct animation_track track;
};

i32 handle_tsj(str8 in_path, str8 out_path, struct alloc scratch);
