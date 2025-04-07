#pragma once

#include "physics/physics.h"
#include "sys-types.h"

struct pinb_reactive_impulse {
	f32 magnitude;
	bool32 normalize;
};

struct pinb_reactive_sprite_offset {
	f32 delay;
	f32 magnitude;
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

struct pinb_animator {
	bool32 play_on_start;
	i32 initial_animation;
};

struct pinb_flipper {
	i32 velocity_easing_function;
	f32 velocity_radius_max;
	f32 velocity_radius_min;
	f32 velocity_scale;
};

struct pinb_flip {
	i32 type;
	bool32 is_enabled;
};

struct pinb_switch {
	bool32 is_enabled;
	bool32 value;
	i32 animation_on;
	i32 animation_off;
};

struct pinb_switch_list {
	i32 next;
	i32 prev;
};

struct pinb_sensor {
	bool32 is_enabled;
	struct col_shape shape;
};

struct pinb_gravity {
	f32 value;
};

struct pinb_sfx_sequence {
	i32 type;
	f32 reset_time;
	f32 vol_min;
	f32 vol_max;
	f32 pitch_min;
	f32 pitch_max;
	usize clips_len;
	str8 clips[10];
};

struct pinb_entity {
	i32 id;
	i32 x;
	i32 y;
	struct pinb_spr spr;
	struct body body;
	struct pinb_plunger plunger;
	struct pinb_reactive_impulse reactive_impulse;
	struct pinb_reactive_sprite_offset reactive_sprite_offset;
	struct pinb_flipper flipper;
	struct pinb_gravity gravity;
	struct pinb_sfx_sequence sfx_sequence;
	struct pinb_animator animator;
	struct pinb_flip flip;
	struct pinb_sensor sensor;
	struct pinb_switch switch_value;
	struct pinb_switch_list switch_list;
};

struct pinb_physics_props {
	u8 steps;
	f32 max_translation; // The maximum translation of a body per time step.
	f32 max_rotation;    // The maximum rotation of a body per time step. This limit is very large and is used to prevent numerical errors
	f32 penetration_correction;
	f32 penetration_allowance;
};

struct pinb_flippers_props {
	f32 flip_velocity;
	f32 rotation_max_turns;
	f32 rotation_min_turns;
	f32 release_velocity;
};

struct pinb_table_props {
	struct pinb_physics_props physics_props;
	struct pinb_flippers_props flippers_props;
};

struct pinb_table {
	usize version;
	struct pinb_table_props props;
	usize entities_count;
	usize entities_max_id;
	struct pinb_entity *entities;
};
