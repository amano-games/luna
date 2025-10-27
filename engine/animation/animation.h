#pragma once

#include "base/types.h"

#define ANIMATION_MAX_FRAMES 50

enum animation_track_type {
	ANIMATION_TRACK_NONE,
	ANIMATION_TRACK_FRAME,
	ANIMATION_TRACK_SPRITE_MODE,
};

struct frames {
	u8 len;
	u8 cap;
	u8 items[ANIMATION_MAX_FRAMES];
};

struct animation_track {
	enum animation_track_type type;
	struct frames frames;
};

struct animation_clip {
	struct animation_track tracks[2];
	f32 clip_duration;
	// used to calculate the total duration
	f32 frame_duration;
	// Play 1 or 2 or 3 times
	// -1 loop forever
	int count;
	// values to sample the current frame
	f32 scale;

	// Cached (frame_duration * scale) / 1
	f32 frame_duration_inv;
};

struct animation {
	u16 is_stopped;
	u16 is_paused;
	f32 timestamp_start;
	f32 timestamp_pause;
	struct animation_clip clip;
};

void animation_init(struct animation *animation);
void animation_clip_init(struct animation_clip *data);
b32 animation_update(struct animation *ani, f32 timestamp);
usize animation_get_frame(struct animation *ani, enum animation_track_type track_type, f32 timestamp);
usize animation_get_frame_index(struct animation *ani, enum animation_track_type track_type, f32 timestamp);
void animation_start(struct animation *ani, f32 timestamp);
void animation_pause(struct animation *ani, f32 timestamp);
void animation_resume(struct animation *ani, f32 timestamp);

f32 animation_data_get_clip_duration(struct animation_clip *data);
