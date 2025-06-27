#include "animation-db.h"
#include "arr.h"
#include "str.h"
#include "sys-utils.h"

void
ani_db_write_clip(struct ser_writer *w, struct animation_clip clip)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("count"));
	ser_write_i32(w, clip.count);

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
ani_db_write_asset(struct ser_writer *w, struct ani_db_asset asset)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("path"));
	ser_write_string(w, asset.path);

	ser_write_string(w, str8_lit("tex_width"));
	ser_write_i32(w, asset.info.tex_size.x);

	ser_write_string(w, str8_lit("tex_height"));
	ser_write_i32(w, asset.info.tex_size.y);

	ser_write_string(w, str8_lit("cell_width"));
	ser_write_i32(w, asset.info.cell_size.x);

	ser_write_string(w, str8_lit("cell_height"));
	ser_write_i32(w, asset.info.cell_size.y);

	ser_write_string(w, str8_lit("clips_count"));
	ser_write_i32(w, arr_len(asset.clips));

	ser_write_string(w, str8_lit("clips"));
	ser_write_array(w);
	for(usize i = 0; i < arr_len(asset.clips); ++i) {
		struct animation_clip clip = asset.clips[i];
		ani_db_write_clip(w, clip);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
ani_db_write(struct ser_writer *w, struct ani_db db)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("clip_count"));
	ser_write_i32(w, db.clip_count);
	ser_write_string(w, str8_lit("bank_count"));
	ser_write_i32(w, db.bank_count);
	ser_write_string(w, str8_lit("assets_count"));
	ser_write_i32(w, arr_len(db.assets));

	ser_write_string(w, str8_lit("assets"));
	ser_write_array(w);
	for(usize i = 0; i < arr_len(db.assets); ++i) {
		ani_db_write_asset(w, db.assets[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

struct animation_track
ani_db_track_read(
	struct ser_reader *r,
	struct ser_value obj)
{
	struct animation_track res = {0};
	struct ser_value key, value;
	res.frames.cap = ARRLEN(res.frames.items);
	while(ser_iter_object(r, obj, &key, &value)) {
		if(str8_match(key.str, str8_lit("len"), 0)) {
			res.frames.len = value.u8;
		} else if(str8_match(key.str, str8_lit("frames"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val)) {
				res.frames.items[i] = item_val.u8;
				i++;
			}
		}
	}
	return res;
}

struct animation_clip
ani_db_clip_read(
	struct ser_reader *r,
	struct ser_value obj,
	struct alloc alloc)
{

	struct animation_clip res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("count"), 0)) {
			res.count = value.i32;
		} else if(str8_match(key.str, str8_lit("frame_duration"), 0)) {
			res.frame_duration = value.f32;
		} else if(str8_match(key.str, str8_lit("scale"), 0)) {
			res.scale = value.f32;
		} else if(str8_match(key.str, str8_lit("tracks"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val)) {
				res.tracks[i]      = ani_db_track_read(r, item_val);
				res.tracks[i].type = i + 1;
				i++;
			}
		}
	}

	dbg_assert(res.count != 0);
	dbg_assert(res.frame_duration > 0);
	dbg_assert(res.frame_duration < 10);
	dbg_assert(res.tracks[0].frames.len > 0 || res.tracks[1].frames.len > 0);

	return res;
}

struct ani_db_asset
ani_db_asset_read(
	struct ser_reader *r,
	struct ser_value obj,
	struct alloc alloc)
{
	struct ani_db_asset res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("path"), 0)) {
			res.path = str8_cpy_push(alloc, value.str);
		} else if(str8_match(key.str, str8_lit("tex_width"), 0)) {
			res.info.tex_size.x = value.i32;
		} else if(str8_match(key.str, str8_lit("tex_height"), 0)) {
			res.info.tex_size.y = value.i32;
		} else if(str8_match(key.str, str8_lit("cell_width"), 0)) {
			res.info.cell_size.x = value.i32;
		} else if(str8_match(key.str, str8_lit("cell_height"), 0)) {
			res.info.cell_size.y = value.i32;
		} else if(str8_match(key.str, str8_lit("clips_count"), 0)) {
			res.clips = arr_new(res.clips, value.i32, alloc);
		} else if(str8_match(key.str, str8_lit("clips"), 0)) {
			struct ser_value item_val;
			while(ser_iter_array(r, value, &item_val)) {
				arr_push(res.clips, ani_db_clip_read(r, item_val, alloc));
			}
		}
	}
	return res;
}

struct ani_db
ani_db_read(struct ser_reader *r, struct alloc alloc)
{
	struct ani_db res   = {0};
	struct ser_value db = ser_read(r);
	struct ser_value key, value;

	while(ser_iter_object(r, db, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("clip_count"), 0)) {
			res.clip_count = value.i32;
		} else if(str8_match(key.str, str8_lit("bank_count"), 0)) {
			res.bank_count = value.i32;
		} else if(str8_match(key.str, str8_lit("assets_count"), 0)) {
			res.assets = arr_new(res.assets, value.i32, alloc);
		} else if(str8_match(key.str, str8_lit("assets"), 0)) {
			struct ser_value item_val;
			while(ser_iter_array(r, value, &item_val)) {
				arr_push(res.assets, ani_db_asset_read(r, item_val, alloc));
			}
		}
	}

	return res;
}
