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

struct animation_data_bank_handle
animation_db_bank_handle_from_path(char *path)
{
	return (struct animation_data_bank_handle){
		.id = hash_string(str8_cstr(path)),
	};
}

struct animation_data_bank *
animation_db_get_bank(struct assets_db *db, struct animation_data_bank_handle handle)
{
	TRACE_START(__func__);
	// TODO: make a hash table instead of this
	for(usize i = 0; i < arr_len(db->banks); ++i) {
		if(db->banks[i].id == handle.id) {
			TRACE_END();
			return &db->banks[i];
		}
	}
	TRACE_END();
	return NULL;
}

struct animation_data
animation_db_get_clip(struct assets_db *db, struct animation_data_bank_handle handle, usize index)
{
	TRACE_START(__func__);
	struct animation_data res        = {0};
	struct animation_data_bank *bank = animation_db_get_bank(db, handle);

	if(bank != NULL) {
		assert(index <= bank->len);
		res = db->clips[bank->index + (index - 1)];
	}

	TRACE_END();
	return res;
}
