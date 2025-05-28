#pragma once

#include "physics/physics.h"
#include "sys-types.h"

struct pinb_reactive_impulse {
	f32 magnitude;
	bool32 normalize;
};

struct pinb_reactive_animation {
	i32 animation_index;
};

struct pinb_reactive_sprite_offset {
	f32 delay;
	f32 magnitude;
	i32 ref;
};

struct pinb_charged_impulse {
	f32 angle;
	f32 magnitude;
	f32 charge_speed;
	f32 release_speed;
	bool32 reset_target;
	bool32 auto_shoot;
};

struct pinb_plunger {
	f32 charge_force_min;
	f32 charge_force_max;
	f32 release_force_min;
	f32 release_force_max;
};

struct pinb_force_field {
	f32 angle;
	f32 magnitude;
	bool32 is_enabled;
};

struct pinb_spr {
	str8 path;
	i32 flip;
	v2 offset;
};

struct pinb_animator_transition {
	i32 from;
	i32 to;
};

struct pinb_animator_transitions {
	size len;
	struct pinb_animator_transition *items;
};

struct pinb_animator {
	bool32 play_on_start;
	i32 initial_animation;
	struct pinb_animator_transitions transitions;
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
	struct col_shapes shapes;
};

struct pinb_gravity {
	f32 value;
};

struct pinb_counter {
	i32 min;
	i32 max;
	i32 value;
	i32 resolution;
};

struct pinb_sfx_sequence {
	i32 type;
	f32 reset_time;
	f32 vol_min;
	f32 vol_max;
	f32 pitch_min;
	f32 pitch_max;
	usize clips_len;
	str8 *clips;
};

struct pinb_sfx_sequences {
	usize len;
	struct pinb_sfx_sequence *items;
};

struct pinb_message {
	i32 sequence_type;
	f32 sequence_reset_time;
	f32 hide_time;
	usize text_len;
	str8 *text;
};

struct pinb_messages {
	usize len;
	struct pinb_message *items;
};

struct pinb_action {
	bool32 debug;
	i32 event_type;
	i32 event_condition; // NOTE: should events be an array?
	i32 event_condition_type;
	i32 action_type;
	i32 action_ref;
	i32 action_arg; // NOTE: Should arguments be an array?
};

struct pinb_actions {
	usize len;
	struct pinb_action *items;
};

struct pinb_spinner {
	f32 damping;
	f32 spin_force;
	f32 stop_threshold;
};

struct pinb_reset {
	i32 flags;
};

struct pinb_entity {
	i32 id;
	i32 x;
	i32 y;
	struct pinb_spr spr;
	struct body body;
	struct pinb_plunger plunger;
	struct pinb_charged_impulse charged_impulse;
	struct pinb_spinner spinner;
	struct pinb_reactive_impulse reactive_impulse;
	struct pinb_reactive_sprite_offset reactive_sprite_offset;
	struct pinb_reactive_animation reactive_animation;
	struct pinb_flipper flipper;
	struct pinb_gravity gravity;
	struct pinb_force_field force_field;
	struct pinb_animator animator;
	struct pinb_flip flip;
	struct pinb_sensor sensor;
	struct pinb_switch switch_value;
	struct pinb_switch_list switch_list;
	struct pinb_sfx_sequences sfx_sequences;
	struct pinb_messages messages;
	struct pinb_actions actions;
	struct pinb_reset reset;
	struct pinb_counter counter;
	struct v2_i32 score_fx_offset;
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
