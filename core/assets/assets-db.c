#include "assets-db.h"

#include "animation/animation.h"
#include "arr.h"
#include "ht.h"
#include "str.h"
#include "trace.h"

#include "./assets-db-parser.h"

void
assets_db_parse(struct assets_db *db, str8 file_name, struct alloc alloc, struct alloc scratch)
{
	asset_db_parser_do(db, file_name, alloc, scratch);

	// Set the tracks to the correct type
	for(usize i = 0; i < arr_len(db->clips); i++) {
		animation_clip_init(&db->clips[i]);
	}
}

void
assets_db_init(struct assets_db *db, usize clip_count, usize slice_count, struct alloc alloc)
{
	db->clips     = arr_ini(clip_count, sizeof(*db->clips), alloc);
	db->slices    = arr_ini(slice_count, sizeof(*db->slices), alloc);
	db->paths.ht  = ht_new(10, alloc);
	db->paths.buf = alloc.allocf(alloc.ctx, sizeof(char) * 10000);
}

struct asset_handle
assets_db_handle_from_path(str8 path, enum asset_type type)
{
	return (struct asset_handle){
		.path_hash = hash_string(path),
		.type      = type,
	};
}

void
assets_db_push_animation_clip(struct assets_db *db, struct animation_clip clip)
{
	arr_push(db->clips, clip);
	animation_clip_init(&db->clips[arr_len(db->clips) - 1]);
}

void
assets_db_push_animation_clip_slice(struct assets_db *db, struct animation_clips_slice slice)
{
	arr_push(db->slices, slice);
	animation_clip_init(&db->clips[arr_len(db->clips) - 1]);
}

struct animation_clips_slice *
animation_db_get_clips_slice(struct assets_db *db, struct asset_handle handle)
{
	TRACE_START(__func__);
	// TODO: make a hash table instead of this
	for(usize i = 0; i < arr_len(db->slices); ++i) {
		if(db->slices[i].path_hash == handle.path_hash) {
			TRACE_END();
			return &db->slices[i];
		}
	}
	TRACE_END();
	return NULL;
}

struct animation_clip
assets_db_get_animation_clip(struct assets_db *db, struct asset_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_clip res           = {0};
	struct animation_clips_slice *slice = animation_db_get_clips_slice(db, handle);

	if(slice != NULL) {
		usize abs_index = slice->index + index;
		assert(index <= slice->len);
		assert(abs_index <= arr_len(db->clips));
		res = db->clips[abs_index];
	}

	TRACE_END();
	return res;
}
