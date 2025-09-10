#pragma once

#include "engine/assets/asset-db.h"
#include "engine/animation/animation.h"

#define MAX_ANIMATIONS 10

struct animator {
	b32 play_on_start;
	usize index;
	struct animation animation;
	struct asset_handle clips_handle;
	usize transitions[MAX_ANIMATIONS];
};

void animator_init(struct animator *animator, f32 timestamp);
b32 animator_update(struct animator *animator, f32 timestamp);
usize animator_get_frame(struct animator *animator, enum animation_track_type type, f32 timestamp);
void animator_play_animation(struct animator *animator, usize index, f32 timestamp);
