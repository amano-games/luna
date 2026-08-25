#pragma once

#include "engine/assets/asset-db.h"
#include "engine/animation/animation.h"
#include "engine/assets/assets.h"

#define ANIMATOR_TRANSITIONS_MAX 11

struct animator {
	b8 play_on_start;
	u8 index;
	struct animation animation;
	struct animation_clip clips[ANIMATOR_TRANSITIONS_MAX];
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

static inline b32
animator_clips_set(struct animator *animator, struct asset_handle clips_handle)
{
	struct animation_slice slice = asset_db_animation_slice_get(&ASSETS.db, clips_handle);
	dbg_assert((u32)animator->index <= slice.size);
	dbg_assert(slice.size < (ssize)ARRLEN(animator->clips));
	for(ssize i = 0; i < (ssize)slice.size; ++i) {
		mcpy_struct(&animator->clips[i], slice.clip + i);
	}
	return true;
}
