#pragma once

#include "sys-types.h"
#include "pinb/pinb.h"

struct pinb_entity_res {
	struct pinb_entity entity;
	usize token_count;
};

struct pinb_rigid_body_res {
	struct body body;
	usize token_count;
};

struct pinb_col_shape_res {
	struct col_shape shapes[10];
	usize shape_count;
	usize token_count;
};

#define PINB_EXT "pinb"

i32 pinbtjson_handle(str8 in_path, str8 out_path);
