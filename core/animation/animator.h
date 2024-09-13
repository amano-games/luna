#pragma once

#include "animation/animation-db.h"
#include "animation/animation.h"

#define MAX_ANIMATIONS 10

struct animator {
	bool32 play_on_start;
	usize index;
	struct animation animation;
	struct animation_data_bank_handle clips;
	usize transitions[MAX_ANIMATIONS];
};

void animator_init(struct animator *animator, f32 timestamp);
bool32 animator_update(struct animator *animator, f32 timestamp, f32 debug);
usize animator_get_frame(struct animator *animator, enum animation_track_type type, f32 timestamp, bool32 debug);
void animator_play_animation(struct animator *animator, usize index, f32 timestamp);
