#pragma once

#include "bet/bet.h"
#include "base/types.h"

struct prop_res {
	usize token_count;
	struct bet_prop prop;
};

struct node_res {
	usize token_count;
	u8 node_index;
};

int handle_btree(str8 in_path, str8 out_path, struct alloc scratch);
