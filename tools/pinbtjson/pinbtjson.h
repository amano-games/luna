#pragma once

#include "sys-types.h"
#include "pinb/pinb.h"

struct pinb_col_shapes {
	usize count;
	struct col_shape items[10];
};

struct pinbtjson_res {
	usize token_count;
	union {
		struct pinb_entity entity;
		struct body body;
		struct col_cir cir;
		struct pinb_flipper flipper;
		struct pinb_sfx_sequence sfx_sequence;
		struct pinb_physics_props physics_props;
		struct pinb_flippers_props flipper_props;
		struct pinb_table_props table_props;
		struct pinb_col_shapes col_shapes;
		struct pinb_spr spr;
		struct pinb_flip flip;
		struct pinb_plunger plunger;
		struct pinb_reactive_impulse reactive_impulse;
		struct pinb_reactive_sprite_offset reactive_sprite_offset;
		struct pinb_animator animator;
		struct pinb_gravity gravity;
	};
};

#define PINB_EXT "pinb"

i32 pinbtjson_handle(str8 in_path, str8 out_path);
