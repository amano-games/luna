#include "animation-clips.h"

#include "base/arr.h"
#include "base/dbg.h"
#include "base/str.h"
#include "base/utils.h"

static void
ani_write_clip(struct ser_writer *w, struct animation_clip clip)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("count"));
	ser_write_i32(w, clip.count);

	dbg_assert(clip.frame_duration > 0);
	dbg_assert(clip.frame_duration < 10);
	ser_write_string(w, str8_lit("frame_duration"));
	ser_write_f32(w, clip.frame_duration);

	ser_write_string(w, str8_lit("scale"));
	ser_write_f32(w, clip.scale);

	ser_write_string(w, str8_lit("tracks"));
	ser_write_array(w);
	for(usize i = 0; i < ARRLEN(clip.tracks); ++i) {
		struct animation_track track = clip.tracks[i];
		ser_write_object(w);

		ser_write_string(w, str8_lit("len"));
		ser_write_u8(w, track.frames.len);

		{
			ser_write_string(w, str8_lit("frames"));
			ser_write_array(w);
			for(usize j = 0; j < track.frames.len; ++j) {
				ser_write_u8(w, track.frames.items[j]);
			}
			ser_write_end(w);
		}

		ser_write_end(w);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
ani_clips_write(struct ser_writer *w, struct animation_clip *clips)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("clips_count"));
	ser_write_i32(w, arr_len(clips));

	ser_write_string(w, str8_lit("clips"));
	ser_write_array(w);
	for(ssize i = 0; i < arr_len(clips); ++i) {
		ani_write_clip(w, clips[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

static struct animation_track
ani_track_read(struct ser_reader *r, struct ser_value obj)
{
	struct animation_track res = {0};
	struct ser_value key, value;

	res.frames.cap = ARRLEN(res.frames.items);
	while(ser_iter_object(r, obj, &key, &value)) {
		if(str8_match(key.str, str8_lit("len"), 0)) {
			res.frames.len = ser_get_u8(value);
		} else if(str8_match(key.str, str8_lit("frames"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val)) {
				res.frames.items[i] = ser_get_u8(item_val);
				i++;
			}
		}
	}

	return res;
}

static struct animation_clip
ani_clip_read(struct ser_reader *r, struct ser_value obj)
{
	struct animation_clip res = {0};
	struct ser_value key, value;

	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("count"), 0)) {
			res.count = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("frame_duration"), 0)) {
			res.frame_duration = ser_get_f32(value);
			dbg_assert(res.frame_duration > 0);
			dbg_assert(res.frame_duration < 10);
		} else if(str8_match(key.str, str8_lit("scale"), 0)) {
			res.scale = ser_get_f32(value);
		} else if(str8_match(key.str, str8_lit("tracks"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val)) {
				res.tracks[i]      = ani_track_read(r, item_val);
				res.tracks[i].type = i + 1;
				i++;
			}
		}
	}

	dbg_assert(res.frame_duration > 0);
	dbg_assert(res.frame_duration < 10);
	dbg_assert(res.tracks[0].frames.len > 0 || res.tracks[1].frames.len > 0);

	return res;
}

struct animation_clip *
ani_clips_read(struct ser_reader *r, struct alloc alloc)
{
	struct animation_clip *res = NULL;
	struct ser_value db        = ser_read(r);
	struct ser_value key, value;

	dbg_assert(db.type == SER_TYPE_OBJECT);

	while(ser_iter_object(r, db, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("clips_count"), 0)) {
			res = arr_new(alloc, res, ser_get_i32(value));
		} else if(str8_match(key.str, str8_lit("clips"), 0)) {
			struct ser_value item_val;
			while(ser_iter_array(r, value, &item_val)) {
				arr_push(res, ani_clip_read(r, item_val));
			}
		}
	}

	return res;
}
