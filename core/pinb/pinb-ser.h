#pragma once

#include "core/pinb/pinb.h"
#include "core/serialize/serialize.h"

i32 pinb_write(struct ser_writer *w, struct pinb_table *pinb);
i32 pinb_read(struct ser_reader *r, struct pinb_table *table, struct alloc alloc);
i32 pinb_inspect(str8 path, struct ser_reader *r, struct alloc alloc);

void pinb_entity_write(struct ser_writer *w, struct pinb_entity *entity);
struct pinb_entity pinb_entity_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);
struct pinb_entity pinb_entity_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_ball_write(struct ser_writer *w, struct pinb_entity *entity);
struct pinb_ball pinb_ball_read(struct ser_reader *r, struct ser_value obj);

void pinb_entity_spr_write(struct ser_writer *w, struct pinb_spr *spr);
struct pinb_spr pinb_spr_read(struct ser_reader *r, struct ser_value obj);

void pinb_counter_write(struct ser_writer *w, struct pinb_counter *value);
struct pinb_counter pinb_counter_read(struct ser_reader *r, struct ser_value obj);

void pinb_sensor_write(struct ser_writer *w, struct pinb_sensor *value);
struct pinb_sensor pinb_sensor_read(struct ser_reader *r, struct ser_value obj);

void pinb_reactive_impulse_write(struct ser_writer *w, struct pinb_reactive_impulse *value);
struct pinb_reactive_impulse pinb_reactive_impulse_read(struct ser_reader *r, struct ser_value obj);

void pinb_force_field_write(struct ser_writer *w, struct pinb_force_field *value);
struct pinb_force_field pinb_force_field_read(struct ser_reader *r, struct ser_value obj);

void pinb_attractor_write(struct ser_writer *w, struct pinb_attractor *value);
struct pinb_attractor pinb_attractor_read(struct ser_reader *r, struct ser_value obj);

void pinb_reactive_sprite_offset_write(struct ser_writer *w, struct pinb_reactive_sprite_offset *value);
struct pinb_reactive_sprite_offset pinb_reactive_sprite_offset_read(struct ser_reader *r, struct ser_value obj);

void pinb_reactive_animation_write(struct ser_writer *w, struct pinb_reactive_animation *value);
struct pinb_reactive_animation pinb_reactive_animation_read(struct ser_reader *r, struct ser_value obj);

void pinb_charged_impulse_write(struct ser_writer *w, struct pinb_charged_impulse *value);
struct pinb_charged_impulse pinb_charged_impulse_read(struct ser_reader *r, struct ser_value obj);

void pinb_plunger_write(struct ser_writer *w, struct pinb_plunger *value);
struct pinb_plunger pinb_plunger_read(struct ser_reader *r, struct ser_value obj);

void pinb_spinner_write(struct ser_writer *w, struct pinb_spinner *value);
struct pinb_spinner pinb_spinner_read(struct ser_reader *r, struct ser_value obj);

void pinb_bucket_write(struct ser_writer *w, struct pinb_bucket *value);
struct pinb_bucket pinb_bucket_read(struct ser_reader *r, struct ser_value obj);

void pinb_ball_saver_write(struct ser_writer *w, struct pinb_ball_saver *value);
struct pinb_ball_saver pinb_ball_saver_read(struct ser_reader *r, struct ser_value obj);

void pinb_flipper_write(struct ser_writer *w, struct pinb_flipper *value);
struct pinb_flipper pinb_flipper_read(struct ser_reader *r, struct ser_value obj);

void pinb_flip_write(struct ser_writer *w, struct pinb_flip *value);
struct pinb_flip pinb_flip_read(struct ser_reader *r, struct ser_value obj);

void pinb_gravity_write(struct ser_writer *w, struct pinb_gravity *value);
struct pinb_gravity pinb_gravity_read(struct ser_reader *r, struct ser_value obj);

void pinb_collision_layer_write(struct ser_writer *w, struct pinb_collision_layer *value);
struct pinb_collision_layer pinb_collision_layer_read(struct ser_reader *r, struct ser_value obj);

void pinb_crank_animation_write(struct ser_writer *w, struct pinb_crank_animation *value);
struct pinb_crank_animation pinb_crank_animation_read(struct ser_reader *r, struct ser_value obj);

void pinb_reset_write(struct ser_writer *w, struct pinb_reset *value);
struct pinb_reset pinb_reset_read(struct ser_reader *r, struct ser_value obj);

void pinb_animator_write(struct ser_writer *w, struct pinb_animator *value);
struct pinb_animator pinb_animator_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_animator_transitions_write(struct ser_writer *w, struct pinb_animator_transitions *value);

void pinb_switch_value_write(struct ser_writer *w, struct pinb_switch *value);
struct pinb_switch pinb_switch_value_read(struct ser_reader *r, struct ser_value obj);

void pinb_entity_list_write(struct ser_writer *w, struct pinb_entity_list *value);
struct pinb_entity_list pinb_entity_list_read(struct ser_reader *r, struct ser_value obj);

void pinb_spawner_write(struct ser_writer *w, struct pinb_spawner *value);
struct pinb_spawner pinb_spawner_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_spawn_zone_write(struct ser_writer *w, struct pinb_spawn_zone *value);
struct pinb_spawn_zone pinb_spawn_zone_read(struct ser_reader *r, struct ser_value obj);

void pinb_mover_write(struct ser_writer *w, struct pinb_mover *value);
struct pinb_mover pinb_mover_read(struct ser_reader *r, struct ser_value obj);

void pinb_mover_path_write(struct ser_writer *w, struct pinb_mover_path *value);
struct pinb_mover_path pinb_mover_path_read(struct ser_reader *r, struct ser_value obj);

void pinb_sfx_sequences_write(struct ser_writer *w, struct pinb_sfx_sequences *value);

struct pinb_sfx_sequence pinb_sfx_sequence_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);
struct pinb_sfx_sequences pinb_sfx_sequences_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_sfx_sequence_write(struct ser_writer *w, struct pinb_sfx_sequence *value);

void pinb_messages_write(struct ser_writer *w, struct pinb_messages *value);
struct pinb_messages pinb_messages_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_message_write(struct ser_writer *w, struct pinb_message *value);
struct pinb_message pinb_message_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_actions_write(struct ser_writer *w, struct pinb_actions *value);
struct pinb_actions pinb_actions_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_action_write(struct ser_writer *w, struct pinb_action *value);
struct pinb_action pinb_action_read(struct ser_reader *r, struct ser_value obj);

void pinb_custom_data_write(struct ser_writer *w, struct pinb_custom_data *value);
struct pinb_custom_data pinb_custom_data_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

void pinb_prop_write(struct ser_writer *w, struct pinb_prop *value);
struct pinb_prop pinb_prop_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);

struct pinb_table_props pinb_table_props_read(struct ser_reader *r, struct ser_value obj);

void pinb_table_switcher_write(struct ser_writer *w, struct pinb_table_switcher *value);
struct pinb_table_switcher pinb_table_switcher_read(struct ser_reader *r, struct ser_value obj);
