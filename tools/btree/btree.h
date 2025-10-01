#pragma once

#include "lib/bet/bet.h"
#include "base/types.h"

struct prop_res {
	usize token_count;
	struct bet_prop prop;
};

struct node_res {
	usize token_count;
	u8 node_index;
};

struct bet_node_holder {
	struct bet_node *nodes;
};

int handle_btree(str8 in_path, str8 out_path, struct alloc scratch);
