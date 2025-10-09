#pragma once

#include "base/types.h"
#include "engine/physics/physics.h"

enum pinb_prop_type {
	PINB_PROP_TYPE_NONE,

	PINB_PROP_TYPE_I32,
	PINB_PROP_TYPE_F32,
	PINB_PROP_TYPE_STR,

	PINB_PROP_TYPE_NUM_COUNT,
};

struct pinb_prop {
	u8 type;
	union {
		i32 i32;
		f32 f32;
		str8 str;
	};
};

struct pinb_custom_data {
	size len;
	struct pinb_prop *data;
};

struct pinb_ball {
	f32 debug_linear_damping;
};

struct pinb_reactive_impulse {
	f32 magnitude;
	b32 normalize;
	f32 cooldown;
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
	f32 angle_rad;
	f32 magnitude;
	f32 charge_speed;
	f32 release_speed;
	b32 reset_target;
	b32 auto_shoot;
	f32 auto_shoot_hold;
};

struct pinb_plunger {
	f32 charge_force_min;
	f32 charge_force_max;
	f32 release_force_min;
	f32 release_force_max;
};

struct pinb_force_field {
	f32 angle_rad;
	f32 magnitude;
	b32 is_enabled;
};

struct pinb_attractor {
	i32 flags;
	v2 offset;
	f32 radius;
	f32 force;
	f32 damping;
	f32 distance_threshold;
};

struct pinb_spr {
	str8 path;
	i32 flip;
	i32 layer;
	i32 y_sort_offset;
	b32 y_sort;
	v2 offset;
};

struct pinb_bet {
	b32 is_enabled;
	str8 path;
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
	b32 play_on_start;
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
	b32 is_enabled;
};

struct pinb_switch {
	b32 is_enabled;
	b32 value;
	i32 animation_on;
	i32 animation_off;
};

struct pinb_entity_list {
	i32 next;
	i32 prev;
};

struct pinb_sensor {
	b32 is_enabled;
	struct col_shapes shapes;
};

struct pinb_gravity {
	f32 value;
};

struct pinb_counter {
	b32 is_enabled;
	i32 type;
	i32 min;
	i32 max;
	i32 value;
	i32 value_initial;
	i32 resolution;
};

struct pinb_crank_animation {
	f32 interval;
};

struct pinb_sfx_sequence {
	i32 type;
	f32 reset_time;
	f32 vol_min;
	f32 vol_max;
	f32 pitch_min;
	f32 pitch_max;
	size clips_len;
	str8 *clips;
};

struct pinb_sfx_sequences {
	size len;
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
	size len;
	struct pinb_message *items;
};

struct pinb_action {
	b32 debug;
	i32 event_type;
	i32 event_condition; // NOTE: should events be an array?
	i32 event_condition_type;
	i32 action_type;
	i32 action_ref;
	f32 action_delay;
	f32 action_cooldown;
	i32 action_arg; // NOTE: Should arguments be an array?
};

struct pinb_actions {
	size len;
	struct pinb_action *items;
};

struct pinb_spinner {
	f32 damping;
	f32 spin_force;
	f32 stop_threshold;
};

struct pinb_bucket {
	i32 animation_shoot;
	i32 animation_on;
	i32 animation_off;
	f32 delay;
	f32 impulse_angle_rad;
	f32 impulse_magnitude;
};

struct pinb_reset {
	i32 flags;
};

struct pinb_collision_layer {
	i32 layer;
};

struct pinb_ball_saver {
	b32 is_enabled;
	f32 duration;
	f32 save_delay;
};

struct pinb_mover {
	i32 flags;
	i32 ref;
	f32 speed;
};

enum pinb_mover_path_type {
	PINB_MOVER_PATH_TYPE_NONE,

	PINB_MOVER_PATH_TYPE_POINT,
	PINB_MOVER_PATH_TYPE_CIR,
	PINB_MOVER_PATH_TYPE_AABB,
	PINB_MOVER_PATH_TYPE_ELLIPSIS,
	PINB_MOVER_PATH_TYPE_LINE,
};

struct pinb_mover_path {
	enum pinb_mover_path_type type;
	union {
		v2 point;
		struct col_cir cir;
		struct col_aabb aabb;
		struct col_ellipsis ellipsis;
		struct col_line line;
	};
};

enum pinb_spawn_zone_type {
	PINB_SPAWN_ZONE_TYPE_NONE,
	PINB_SPAWN_ZONE_TYPE_CIR,
	PINB_SPAWN_ZONE_TYPE_AABB,
	PINB_SPAWN_ZONE_TYPE_POINT,
};

struct pinb_spawn_zone {
	i32 mode;
	i32 capacity;
	enum pinb_spawn_zone_type type;
	union {
		v2 point;
		struct col_cir cir;
		struct col_aabb aabb;
	};
};

struct pinb_table_switcher {
	i32 table;
};

struct pinb_spawner {
	i32 ref;
	i32 type;
	i32 zones_len;
	i32 *zones;
};

struct pinb_entity {
	i32 id;
	i32 flags;
	i32 x;
	i32 y;
	struct pinb_ball ball;
	struct pinb_spr spr;
	struct pinb_bet bet;
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
	struct pinb_attractor attractor;
	struct pinb_animator animator;
	struct pinb_flip flip;
	struct pinb_sensor sensor;
	struct pinb_switch switch_value;
	struct pinb_entity_list entity_list;
	struct pinb_sfx_sequences sfx_sequences;
	struct pinb_reset reset;
	struct pinb_counter counter;
	struct pinb_crank_animation crank_animation;
	struct pinb_bucket bucket;
	struct pinb_ball_saver ball_saver;
	struct pinb_table_switcher table_switcher;
	struct pinb_mover mover;
	struct pinb_mover_path mover_path;
	struct v2_i32 score_fx_offset;
	struct pinb_collision_layer collision_layer;
	struct pinb_spawner spawner;
	struct pinb_spawn_zone spawn_zone;
	struct pinb_messages messages;
	struct pinb_actions actions;
	struct pinb_custom_data custom_data;
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
	str8 bg_tex_path;
	u64 score;
	i32 balls;
	i32 balls_max;
	i32 score_mult;
	i32 score_mult_max;
	struct pinb_physics_props physics_props;
	struct pinb_flippers_props flippers_props;
};

struct pinb_table {
	u32 flags;
	usize version;
	struct pinb_table_props props;
	usize entities_count;
	usize entities_max_id;
	struct pinb_entity *entities;
};
