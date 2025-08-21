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
		struct col_aabb aabb;
		struct pinb_flipper flipper;
		struct pinb_physics_props physics_props;
		struct pinb_flippers_props flipper_props;
		struct pinb_table_props table_props;
		struct pinb_col_shapes col_shapes;
		struct pinb_spr spr;
		struct pinb_flip flip;
		struct pinb_plunger plunger;
		struct pinb_charged_impulse charged_impulse;
		struct pinb_spinner spinner;
		struct pinb_force_field force_field;
		struct pinb_attractor attractor;
		struct pinb_reactive_impulse reactive_impulse;
		struct pinb_reactive_sprite_offset reactive_sprite_offset;
		struct pinb_reactive_animation reactive_animation;
		struct pinb_animator animator;
		struct pinb_animator_transition animator_transition;
		struct pinb_gravity gravity;
		struct pinb_counter counter;
		struct pinb_sensor sensor;
		struct pinb_switch switch_value;
		struct pinb_entity_list entity_list;
		struct pinb_sfx_sequence sfx_sequence;
		struct pinb_action action;
		struct pinb_message message;
		struct pinb_reset reset;
		struct pinb_bucket bucket;
		struct pinb_crank_animation crank_animation;
		struct pinb_collision_layer collision_layer;
		struct pinb_ball_saver ball_saver;
		struct pinb_spawner spawner;
		struct pinb_spawn_zone spawn_zone;
		struct pinb_table_switcher table_switcher;
	};
};

#define PINB_EXT "pinb"

i32 pinbtjson_handle(str8 in_path, str8 out_path);
