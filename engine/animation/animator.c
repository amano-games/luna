#include "animator.h"

#include "engine/assets/asset-db.h"
#include "engine/animation/animation.h"

#include "engine/assets/assets.h"

static inline void animator_animation_set(struct animator *animator, usize index);

void
animator_ini(struct animator *animator)
{
	dbg_assert(animator->idx != 0);
	animator->animation.clip = animator->clips[animator->idx - 1];
}

b32
animator_update(struct animator *animator, struct tick_ms timestamp)
{
	b32 res              = false;
	u8 current_animation = animator->idx;
	res                  = animation_update(&animator->animation, timestamp);
	return res;
}

void
animator_animation_play(struct animator *animator, usize index, struct tick_ms timestamp)
{
	dbg_assert(index > 0);
	dbg_assert(index <= ARRLEN(animator->clips));
	if(index != animator->idx) {
		animator_animation_set(animator, index);
	}
	animation_start(&animator->animation, timestamp);
}

void
animator_animation_pause(struct animator *animator, struct tick_ms timestamp)
{
	animation_pause(&animator->animation, timestamp);
}

void
animator_animation_resume(struct animator *animator, struct tick_ms timestamp)
{
	animation_resume(&animator->animation, timestamp);
}

static inline void
animator_animation_set(struct animator *animator, usize idx)
{
	dbg_assert(idx != 0);
	animator->idx            = idx;
	animator->animation.clip = animator->clips[idx - 1];
	animation_init(&animator->animation);
}
