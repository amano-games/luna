#include "animator.h"

#include "assets/asset-db.h"
#include "animation/animation.h"

#include "assets/assets.h"
#include "trace.h"

void animator_set_animation(struct animator *animator, usize index);

void
animator_init(struct animator *animator, f32 timestamp)
{
	struct animation *animation = &animator->animation;

	animator_set_animation(animator, animator->index);
	if(animator->play_on_start) {
		animator_play_animation(animator, animator->index, timestamp);
	}
}

b32
animator_update(struct animator *animator, f32 timestamp)
{
	TRACE_START(__func__);
	b32 res                      = false;
	usize current_animation      = animator->index;
	struct animation *animation  = &animator->animation;
	struct animation_slice slice = asset_db_animation_slice_get(&ASSETS.db, animator->clips_handle);

	if(slice.size > 0) {
		res = animation_update(animation, timestamp);
	}
	TRACE_END();
	return res;
}

usize
animator_get_frame(struct animator *animator, enum animation_track_type type, f32 timestamp)
{
	dbg_assert(type == ANIMATION_TRACK_FRAME || type == ANIMATION_TRACK_SPRITE_MODE);

	struct animation *animation = &animator->animation;
	return animation_get_frame(animation, type, timestamp);
}

void
animator_play_animation(struct animator *animator, usize index, f32 timestamp)
{
	TRACE_START(__func__);
	dbg_assert(index > 0);
	dbg_assert(animator->clips_handle.path_hash != 0);
	struct animation_slice slice = asset_db_animation_slice_get(&ASSETS.db, animator->clips_handle);
	dbg_assert(index <= slice.size);
	if(index != animator->index) {
		animator_set_animation(animator, index);
	}
	animation_start(&animator->animation, timestamp);
	TRACE_END();
}

void
animator_set_animation(struct animator *animator, usize index)
{
	dbg_assert(index != 0);
	animator->index = index;
	// TODO: Replace with function
	animator->animation.clip = asset_db_animation_clip_get(&ASSETS.db, animator->clips_handle, index - 1);
	animation_init(&animator->animation);
}
