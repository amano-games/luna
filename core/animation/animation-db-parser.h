#pragma once

#include "animation/animation-db.h"
#include "animation/animation.h"
#include "sys-types.h"

struct animation_db_info_res {
	usize token_count;
	struct animation_db_info item;
};

struct animation_track_res {
	usize token_count;
	struct animation_track item;
};

struct animation_data_res {
	usize token_count;
	struct animation_data item;
};

struct animation_data_bank_res {
	usize token_count;
	struct animation_data_bank item;
};

struct animation_db_res {
	struct animation_data *clips;
	struct animation_data_bank *banks;
};

void parse_animation_db(struct animation_db *db, char *file_name, struct alloc *alloc, struct alloc *scratch);
