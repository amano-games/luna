#include "animation.h"

#include "base/types.h"
#include "base/utils.h"
#include "base/dbg.h"

#include "base/mathfunc.h"

void
animation_clip_init(struct animation_clip *data)
{
	for(usize i = 0; i < ARRLEN(data->tracks); ++i) {
		struct animation_track *track = &data->tracks[i];
		track->type                   = i + 1;
		if(track->type == ANIMATION_TRACK_FRAME) {
			// dbg_assert(track->frames.len > 0);
		}
		dbg_assert(track->type != ANIMATION_TRACK_NONE);
		dbg_assert(track->type < ANIMATION_TRACK_SPRITE_MODE + 1);
	}
	data->clip_duration      = animation_data_get_clip_duration(data);
	data->frame_duration_inv = 1.0f / (data->frame_duration * data->scale);
}

void
animation_init(struct animation *animation)
{
	animation->is_playing      = false;
	animation->timestamp_start = TICK_MS_ZERO;
}

void
animation_start(struct animation *ani, struct tick_ms timestamp)
{
	ani->is_playing      = true;
	ani->is_paused       = false;
	ani->timestamp_start = timestamp;
	ani->timestamp_pause = TICK_MS_ZERO;
}

void
animation_pause(struct animation *ani, struct tick_ms timestamp)
{
	if(!ani->is_paused) {
		ani->is_paused       = true;
		ani->timestamp_pause = timestamp;
	}
}

void
animation_resume(struct animation *ani, struct tick_ms timestamp)
{
	if(!ani->is_paused) { return; }
	ani->is_paused       = false;
	ani->timestamp_start = tick_ms_add(ani->timestamp_start, tick_ms_diff(timestamp, ani->timestamp_pause));
	ani->timestamp_pause = TICK_MS_ZERO;
}

b32 // finished?
animation_update(struct animation *ani, struct tick_ms timestamp)
{
	b32 res = false;
	if(!ani->is_playing) { goto cleanup; }
	if(ani->is_paused) { goto cleanup; }
	if(ani->clip.count <= 0) { goto cleanup; }

	f32 duration           = ani->clip.clip_duration * ani->clip.count;
	struct tick_ms start   = ani->timestamp_start;
	struct tick_ms current = timestamp;
	f32 delta              = tick_ms_sec_between(current, start);
	ani->is_playing        = delta <= duration;
	res                    = !ani->is_playing;

cleanup:
	return res;
}

usize
animation_get_first_frame(struct animation_track *track)
{
	usize res = 0;
	if(track->type == ANIMATION_TRACK_SPRITE_MODE && track->frames.len == 0) { goto cleanup; }
	dbg_assert(track->frames.len > 0);
	res = track->frames.items[0];

cleanup:
	return res;
}

i32
animation_get_frame_index(struct animation *ani, enum animation_track_type track_type, struct tick_ms timestamp)
{
	dbg_assert(track_type == ANIMATION_TRACK_FRAME || track_type == ANIMATION_TRACK_SPRITE_MODE);

	i32 res                       = 0;
	usize track_index             = track_type - 1;
	struct animation_clip *clip   = &ani->clip;
	struct animation_track *track = &clip->tracks[track_index];

	if(track->frames.len == 0) {
		res = 0;
		goto cleanup;
	}

	if(!ani->is_playing) {
		res = (i32)track->frames.len - 1;
		goto cleanup;
	};

	struct tick_ms start   = ani->timestamp_start;
	struct tick_ms current = ani->is_paused ? ani->timestamp_pause : timestamp;
	f32 delta              = tick_ms_sec_between(current, start);
	b32 loop               = clip->count <= 0;

	if(!loop) {
		delta = min_f32(delta, clip->clip_duration * clip->count);
	}

	res = (i32)(delta * clip->frame_duration_inv);
	if(!loop) {
		// delta == duration lands one past the last frame, keep it on the last one
		res = min_i32(res, (i32)(track->frames.len * clip->count) - 1);
	}
	res = mod_euc_i32(res, track->frames.len);

cleanup:
	return res;
}

u8
animation_get_frame(struct animation *ani, enum animation_track_type track_type, struct tick_ms timestamp)
{
	struct frames *frames = &ani->clip.tracks[track_type - 1].frames;
	if(frames->len == 0) { return 0; }
	return frames->items[animation_get_frame_index(ani, track_type, timestamp)];
}

f32
animation_data_get_clip_duration(struct animation_clip *data)
{
	f32 duration = 0;
	for(usize i = 0; i < ARRLEN(data->tracks); ++i) {
		struct animation_track *track = &data->tracks[i];
		if(track->type != ANIMATION_TRACK_NONE) {
			f32 track_duration = track->frames.len * (data->frame_duration);
			duration           = max_f32(track_duration, duration);
		}
	}
	return duration * data->scale;
}
