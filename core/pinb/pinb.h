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

struct pinb_entity {
	i32 id;
	i32 x;
	i32 y;
	struct pinb_spr spr;
	struct body body;
	struct pinb_plunger plunger;
	struct pinb_reactive_impulse reactive_impulse;
};

struct pinb_table {
	usize version;
	usize entities_count;
	struct pinb_entity *entities;
};
