#include "animation.h"

#include "base/types.h"
#include "base/utils.h"
#include "base/dbg.h"

#include "base/mathfunc.h"
#include "base/trace.h"

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
	animation->is_stopped      = true;
	animation->timestamp_start = 0;
}

void
animation_start(struct animation *ani, f32 timestamp)
{
	ani->is_stopped      = false;
	ani->is_paused       = false;
	ani->timestamp_start = timestamp;
	ani->timestamp_pause = 0;
}

void
animation_pause(struct animation *ani, f32 timestamp)
{
	if(!ani->is_paused) {
		ani->is_paused       = true;
		ani->timestamp_pause = timestamp;
	}
}

void
animation_resume(struct animation *ani, f32 timestamp)
{
	ani->is_paused       = false;
	ani->timestamp_start = timestamp - (timestamp - ani->timestamp_pause);
	ani->timestamp_pause = 0;
}

b32 // finished?
animation_update(struct animation *ani, f32 timestamp)
{
	TRACE_START(__func__);
	b32 res = false;
	if(ani->is_stopped) { goto cleanup; }
	if(ani->is_paused) { goto cleanup; }
	if(ani->clip.count <= 0) { goto cleanup; }

	f32 duration    = ani->clip.clip_duration * ani->clip.count;
	f32 start       = ani->timestamp_start;
	f32 current     = timestamp;
	f32 delta       = current - start;
	f32 end         = start + duration;
	ani->is_stopped = current > end;
	res             = ani->is_stopped;

cleanup:
	TRACE_END();
	return res;
}

usize
animation_get_last_frame(struct animation_track *track)
{
	TRACE_START(__func__);

	usize res = 0;
	if(track->type == ANIMATION_TRACK_SPRITE_MODE && track->frames.len == 0) { goto cleanup; }
	dbg_assert(track->frames.len > 0);
	usize frame_index = track->frames.len - 1;
	res               = track->frames.items[frame_index];

cleanup:
	TRACE_END();
	return res;
}

usize
animation_get_first_frame(struct animation_track *track)
{
	TRACE_START(__func__);
	usize res = 0;
	if(track->type == ANIMATION_TRACK_SPRITE_MODE && track->frames.len == 0) { goto cleanup; }
	dbg_assert(track->frames.len > 0);
	res = track->frames.items[0];

cleanup:
	TRACE_END();
	return res;
}

usize
animation_get_frame_index(struct animation *ani, enum animation_track_type track_type, f32 timestamp)
{
	TRACE_START(__func__);
	usize res = 0;
	dbg_assert(track_type == ANIMATION_TRACK_FRAME || track_type == ANIMATION_TRACK_SPRITE_MODE);
	usize track_index             = track_type - 1;
	struct animation_track *track = &ani->clip.tracks[track_index];

	if(ani->is_stopped) {
		res = track->frames.len - 1;
		goto cleanup;
	};

	if(track->frames.len == 0) {
		res = 0;
		goto cleanup;
	}

	f32 start   = ani->timestamp_start;
	f32 current = ani->is_paused ? ani->timestamp_pause : timestamp;
	f32 delta   = current - start;
	b32 loop    = ani->clip.count <= 0;

	if(!loop) {
		delta = min_f32(delta, ani->clip.clip_duration * ani->clip.count);
	}

	res = delta * ani->clip.frame_duration_inv;

cleanup:
	TRACE_END();
	return res;
}

usize
animation_get_frame(struct animation *ani, enum animation_track_type track_type, f32 timestamp)
{
	TRACE_START(__func__);
	usize res = 0;
	dbg_assert(track_type == ANIMATION_TRACK_FRAME || track_type == ANIMATION_TRACK_SPRITE_MODE);

	usize track_index             = track_type - 1;
	struct animation_track *track = &ani->clip.tracks[track_index];

	if(ani->is_stopped) {
		res = animation_get_last_frame(track);
		goto cleanup;
	};

	if(track->frames.len == 0) {
		res = 0;
		goto cleanup;
	}

	f32 start   = ani->timestamp_start;
	f32 current = ani->is_paused ? ani->timestamp_pause : timestamp;
	f32 delta   = current - start;
	b32 loop    = ani->clip.count <= 0;

	if(!loop) {
		delta = min_f32(delta, ani->clip.clip_duration * ani->clip.count);
	}

	i32 frame_index = delta * ani->clip.frame_duration_inv;
	frame_index     = mod_euc_i32(frame_index, track->frames.len);
	res             = track->frames.items[(usize)frame_index];

cleanup:
	TRACE_END();
	return res;
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
