#pragma once

#include "engine/assets/asset-db.h"
#include "engine/animation/animation.h"

#define ANIMATOR_TRANSITIONS_MAX 11

struct animator {
	b8 play_on_start;
	u8 index;
	struct animation animation;
	struct asset_handle clips_handle;
	u8 transitions[ANIMATOR_TRANSITIONS_MAX];
};

void animator_init(struct animator *animator, f32 timestamp);
b32 animator_update(struct animator *animator, f32 timestamp);
void animator_animation_play(struct animator *animator, usize index, f32 timestamp);
void animator_animation_pause(struct animator *animator, f32 timestamp);
void animator_animation_resume(struct animator *animator, f32 timestamp);

static inline u8
animator_get_frame(struct animator *animator, enum animation_track_type type, f32 timestamp)
{
	return animation_get_frame(&animator->animation, type, timestamp);
}
