#include "animator.h"
#include "animation/animation-db.h"
#include "animation/animation.h"
#include "assets.h"
#include "sys-backend.h"
#include "sys-log.h"
#include "sys-utils.h"
#include "trace.h"
#include <assert.h>

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

bool32
animator_update(struct animator *animator, f32 timestamp, f32 debug)
{
	TRACE_START(__func__);
	usize current_animation          = animator->index;
	struct animation *animation      = &animator->animation;
	struct animation_data_bank *bank = animation_db_get_bank(&ASSETS.animation_db, animator->clips);

	if(bank->len > 0) {
		TRACE_END();
		return animation_update(animation, timestamp, debug);
	}
	TRACE_END();
	return false;
}

usize
animator_get_frame(struct animator *animator, enum animation_track_type type, f32 timestamp, bool32 debug)
{
	assert(type == ANIMATION_TRACK_FRAME || type == ANIMATION_TRACK_SPRITE_MODE);

	struct animation *animation = &animator->animation;
	return animation_get_frame(animation, type, timestamp, debug);
}

void
animator_play_animation(struct animator *animator, usize index, f32 timestamp)
{
	TRACE_START(__func__);
	assert(index != 0);
	struct animation_data_bank *bank = animation_db_get_bank(&ASSETS.animation_db, animator->clips);
	assert(index <= bank->len);
	if(index != animator->index) {
		animator_set_animation(animator, index);
	}
	animation_start(&animator->animation, timestamp);
	TRACE_END();
}

void
animator_set_animation(struct animator *animator, usize index)
{
	assert(index != 0);
	animator->index = index;
	// TODO: Replace with function
	animator->animation.data = animation_db_get_clip(&ASSETS.animation_db, animator->clips, index);
	animation_init(&animator->animation);
}
