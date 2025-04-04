#pragma once

#include "physics/physics.h"
#include "sys-types.h"

struct pinb_reactive_impulse {
	f32 magnitude;
	bool32 normalize;
};

struct pinb_plunger {
	f32 y_initial;
	f32 charge_force_min;
	f32 charge_force_max;
	f32 release_force_min;
	f32 release_force_max;
};

struct pinb_spr {
	str8 path;
	i32 flip;
	v2 offset;
};

struct pinb_flipper {
	i32 flip_type;
};

struct pinb_entity {
	i32 id;
	i32 x;
	i32 y;
	struct pinb_spr spr;
	struct body body;
	struct pinb_plunger plunger;
	struct pinb_reactive_impulse reactive_impulse;
	struct pinb_flipper flipper;
};

struct pinb_flippers_props {
	f32 flip_velocity;
	f32 rotation_max_turns;
	f32 rotation_min_turns;
	f32 release_velocity;
	i32 velocity_easing_function;
	f32 velocity_radius_max;
	f32 velocity_radius_min;
	f32 velocity_scale;
};

struct pinb_table_props {
	struct pinb_flippers_props flippers_props;
};

struct pinb_table {
	usize version;
	struct pinb_table_props props;
	usize entities_count;
	struct pinb_entity *entities;
};
