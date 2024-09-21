#include "animation.h"
#include "mathfunc.h"
#include "sys-log.h"
#include "sys-utils.h"

void
animation_data_init(struct animation_data *data)
{
	for(usize i = 0; i < ARRLEN(data->tracks); ++i) {
		struct animation_track *track = &data->tracks[i];
		track->type                   = i + 1;
		if(track->type == ANIMATION_TRACK_FRAME) {
			// assert(track->frames.len > 0);
		}
		assert(track->type != ANIMATION_TRACK_NONE);
		assert(track->type < ANIMATION_TRACK_SPRITE_MODE + 1);
	}
	data->clip_duration      = animation_data_get_clip_duration(data);
	data->frame_duration_inv = 1.0f / (data->frame_duration * data->scale);
}

void
animation_init(struct animation *animation)
{
	animation->is_stopped      = true;
	animation->timer.timestamp = 0;
}

void
animation_start(struct animation *ani, f32 timestamp)
{
	ani->is_stopped      = false;
	ani->timer.timestamp = timestamp;
}

bool32 // finished?
animation_update(struct animation *ani, f32 timestamp)
{
	if(ani->is_stopped) { return false; }
	if(ani->data.count == -1) { return false; }

	struct animation_timer *timer = &ani->timer;
	f32 duration                  = ani->data.clip_duration * ani->data.count;
	f32 start                     = timer->timestamp;
	f32 current                   = timestamp;
	f32 delta                     = current - start;
	f32 end                       = start + duration;
	ani->is_stopped               = current > end;

	return ani->is_stopped;
}

usize
animation_get_last_frame(struct animation_track *track)
{
	if(track->type == ANIMATION_TRACK_SPRITE_MODE && track->frames.len == 0) return 0;
	assert(track->frames.len > 0);
	usize frame_index = track->frames.len - 1;
	usize res         = track->frames.items[frame_index];
	return res;
}

usize
animation_get_first_frame(struct animation_track *track)
{
	if(track->type == ANIMATION_TRACK_SPRITE_MODE && track->frames.len == 0) return 0;
	assert(track->frames.len > 0);
	usize res = track->frames.items[0];
	return res;
}

usize
animation_get_frame(struct animation *ani, enum animation_track_type track_type, f32 timestamp)
{
	assert(track_type == ANIMATION_TRACK_FRAME || track_type == ANIMATION_TRACK_SPRITE_MODE);

	usize track_index             = track_type - 1;
	struct animation_track *track = &ani->data.tracks[track_index];

	if(ani->is_stopped) {
		return animation_get_last_frame(track);
	};

	if(track->frames.len == 0) {
		return 0;
	}

	struct animation_timer *timer = &ani->timer;

	f32 start   = timer->timestamp;
	f32 current = timestamp;
	f32 delta   = current - start;
	// c32 frame_index = delta / (ani->data.frame_duration * ani->data.scale);
	i32 frame_index = delta * ani->data.frame_duration_inv;

	bool32 loop = ani->data.count == -1;

	if(!loop) {
		frame_index = clamp_i32(frame_index, 0, track->frames.len - 1);
	}

	frame_index = mod_euc_i32(frame_index, track->frames.len);
	usize res   = track->frames.items[(usize)frame_index];

	return res;
}

f32
animation_data_get_clip_duration(struct animation_data *data)
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
