#include "animation-db.h"

#include "ht.h"
#include "jsmn.h"

#include "animation/animation-db-parser.h"
#include "animation/animation.h"
#include "arr.h"
#include "mem-arena.h"
#include "str.h"
#include "trace.h"

void
animation_db_init(struct animation_db *db, struct marena *marena, struct marena *scratch_marena)
{

	struct alloc alloc   = marena_allocator(marena);
	struct alloc scratch = marena_allocator(scratch_marena);
	parse_animation_db(db, "asset-db-animations.luni", &alloc, &scratch);

	// Set the tracks to the correct type
	for(usize i = 0; i < arr_len(db->clips); i++) {
		animation_data_init(&db->clips[i]);
	}
}

struct animation_data_bank_handle
animation_db_bank_handle_from_path(char *path)
{
	return (struct animation_data_bank_handle){
		.id = hash_string(str_from(path)),
	};
}

struct animation_data_bank *
animation_db_get_bank(struct animation_db *db, struct animation_data_bank_handle handle)
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
animation_db_get_clip(struct animation_db *db, struct animation_data_bank_handle handle, usize index)
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
