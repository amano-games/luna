#pragma once

#include "engine/animation/animation.h"
#include "engine/assets/asset-db.h"

struct tsj_tile_res {
	usize token_count;
	str8 path;
	struct animation_clip *clips;
	struct tex_atlas atlas;
	v2_i32 tex_size;
	str8 src_path;
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

i32 handle_tsj(str8 in_path, str8 out_path, str8 src_root, str8 dest_root, struct alloc scratch);
