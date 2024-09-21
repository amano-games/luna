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
		animation_data_init(&db->clips[i]);
	}
}

void
assets_db_init(struct assets_db *db, usize bank_count, usize clip_count, struct alloc alloc)
{
	db->slices = arr_ini(bank_count, sizeof(*db->slices), alloc);
	db->clips  = arr_ini(clip_count, sizeof(*db->clips), alloc);
}

void
assets_db_push_animation_data(struct assets_db *db, struct animation_data clip)
{
	arr_push(db->clips, clip);
	animation_data_init(&db->clips[arr_len(db->clips) - 1]);
}

void
assets_db_push_animation_data_slice(struct assets_db *db, struct animation_data_slice slice)
{
	arr_push(db->slices, slice);
	animation_data_init(&db->clips[arr_len(db->clips) - 1]);
}

struct animation_data_slice_handle
animation_db_bank_handle_from_path(str8 path)
{
	return (struct animation_data_slice_handle){
		.id = hash_string(path),
	};
}

struct animation_data_slice *
animation_db_get_data_slice(struct assets_db *db, struct animation_data_slice_handle handle)
{
	TRACE_START(__func__);
	// TODO: make a hash table instead of this
	for(usize i = 0; i < arr_len(db->slices); ++i) {
		if(db->slices[i].id == handle.id) {
			TRACE_END();
			return &db->slices[i];
		}
	}
	TRACE_END();
	return NULL;
}

struct animation_data
animation_db_get_clip(struct assets_db *db, struct animation_data_slice_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_data res         = {0};
	struct animation_data_slice *bank = animation_db_get_data_slice(db, handle);

	if(bank != NULL) {
		assert(index <= bank->len);
		res = db->clips[bank->index + (index - 1)];
	}

	TRACE_END();
	return res;
}
