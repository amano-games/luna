#include "animator.h"

#include "engine/assets/asset-db.h"
#include "engine/animation/animation.h"

#include "engine/assets/assets.h"

static inline void animator_animation_set(struct animator *animator, usize index);

void
animator_init(struct animator *animator, f32 timestamp)
{
	struct animation *animation = &animator->animation;

	animator_animation_set(animator, animator->index);
	if(animator->play_on_start) {
		animator_animation_play(animator, animator->index, timestamp);
	}
}

b32
animator_update(struct animator *animator, f32 timestamp)
{
	b32 res              = false;
	u8 current_animation = animator->index;
	res                  = animation_update(&animator->animation, timestamp);
	return res;
}

void
animator_animation_play(struct animator *animator, usize index, f32 timestamp)
{
	dbg_assert(index > 0);
	dbg_assert(index <= ARRLEN(animator->clips));
	if(index != animator->index) {
		animator_animation_set(animator, index);
	}
	animation_start(&animator->animation, timestamp);
}

void
animator_animation_pause(struct animator *animator, f32 timestamp)
{
	animation_pause(&animator->animation, timestamp);
}

void
animator_animation_resume(struct animator *animator, f32 timestamp)
{
	animation_resume(&animator->animation, timestamp);
}

static inline void
animator_animation_set(struct animator *animator, usize index)
{
	dbg_assert(index != 0);
	animator->index          = index;
	animator->animation.clip = animator->clips[index - 1];
	animation_init(&animator->animation);
}
